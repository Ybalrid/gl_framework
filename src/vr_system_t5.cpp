#include "vr_system_t5.hpp"

#include "include/TiltFiveNative.hpp"

static constexpr size_t GLASSES_BUFFER_SIZE { 1024 };
static constexpr size_t WAND_BUFFER_SIZE { 4 };
static constexpr size_t PARAM_BUFFER_SIZE { 1024 };

static constexpr int defaultWidth  = 1216;
static constexpr int defaultHeight = 768;
static float defaultFOV            = 48.0;

//TODO handle t5 projection "correctly"
void eye_projection(glm::mat4& p, const float nearz, const float farz)
{
  p = glm::perspectiveFov<float>(glm::radians(defaultFOV), defaultWidth, defaultHeight, nearz, farz);
}

glm::vec3 vr_system_t5::T5_to_GL(const T5_Vec3& pos)
{
  glm::vec3 glPos_GBD { pos.x, pos.z, -pos.y };

  return glPos_GBD;
}

glm::quat vr_system_t5::T5_to_GL(const T5_Quat& rot)
{
  glm::quat glRot_gbd(rot.w, rot.x, rot.y, rot.z);

  glRot_gbd = glm::inverse(glRot_gbd);
  glm::quat rotationFix(-glm::half_pi<float>(), glm::vec3(1.f, 0, 0)); //t5 headset is looking along the Y axis, not the Z axis

  return rotationFix * glRot_gbd;
}

void vr_system_t5::detect_events(const T5_WandReport& currentFrameReport)
{
  if(lastReport.buttonsValid && currentFrameReport.buttonsValid) { }

  if(lastReport.analogValid && currentFrameReport.analogValid) { }
}

vr_system_t5::vr_system_t5()
{
  std::cout << "Initialized TiltFive based vr_system implementation\n";

  caps = caps_hmd_3dof | caps_hmd_6dof | caps_hand_controllers;
}

vr_system_t5::~vr_system_t5()
{
  t5DestroyGlasses(&glassesHandle);
  t5DestroyContext(&t5ctx);
}

bool vr_system_t5::initialize(sdl::Window& window)
{
  std::cout << "Initializing TiltFive based VR system\n";

  T5_Result err = T5_SUCCESS;

  err = t5CreateContext(&t5ctx, &clientInfo, nullptr);
  if(err)
  {
    std::cout << "Failed to create TiltFive context." << t5GetResultMessage(err) << std::endl;
    return false;
  }

  //Get all gameboard sizes we could support
  //TODO checkerror
  err = t5GetGameboardSize(t5ctx, kT5_GameboardType_XE_Raised, &gameboardSizeXERaised);
  err = t5GetGameboardSize(t5ctx, kT5_GameboardType_XE, &gameboardSizeXE);
  err = t5GetGameboardSize(t5ctx, kT5_GameboardType_LE, &gameboardSizeLE);

  {
    bool waiting = false;
    for(;;)
    {
      size_t bufferSize = T5_MAX_STRING_PARAM_LEN;
      err               = t5GetSystemUtf8Param(t5ctx, kT5_ParamSys_UTF8_Service_Version, serviceVersion, &bufferSize);

      if(!err)
      {
        std::cout << "Tilt Five service version : " << serviceVersion << std::endl;
        break; //service found.
      }

      if(T5_ERROR_NO_SERVICE == err)
      {
        if(!waiting)
        {
          std::cout << "Waiting for TiltFive service ... \n";
          //TODO print "waiting for service ..."
          waiting = true;
        }
      }
      else
      {
        const auto errorMessage = t5GetResultMessage(err);
        std::cout << "Failed to obtain TiltFive service version : " << errorMessage << std::endl;
        return false;
      }
    }
  }

  {
    size_t bufferSize = GLASSES_BUFFER_SIZE;
    char glassesListBuffer[GLASSES_BUFFER_SIZE];
    err = t5ListGlasses(t5ctx, glassesListBuffer, &bufferSize);

    if(!err)
    {
      const char* buffPtr = glassesListBuffer;
      for(;;)
      {
        const auto nameLen = strlen(buffPtr);
        if(nameLen == 0) break;

        std::string glassesName { buffPtr, nameLen };
        buffPtr += nameLen;

        glassesSerialNumber.push_back(glassesName);

        if(buffPtr > glassesListBuffer + GLASSES_BUFFER_SIZE)
        {
          std::cout << "WARN: truncated glasses list buffer\n";
          break;
        }
      }
    }

    if(glassesSerialNumber.empty())
    {
      std::cout << "Did not detect any Tilt Five glasses.\n";
      return false;
    }

    std::cout << "found " << glassesSerialNumber.size() << " TiltFive glasses:\n";
    for(const auto& glassesSN : glassesSerialNumber) { std::cout << "\t- " << glassesSN << "\n"; }

    singleUserGlassesSN = glassesSerialNumber.front();

    err = t5CreateGlasses(t5ctx, singleUserGlassesSN.c_str(), &glassesHandle);
    if(err)
    {
      std::cout << "Error creating glasses " << singleUserGlassesSN << ": " << t5GetResultMessage(err);
      return false;
    }

    char glassesName[T5_MAX_STRING_PARAM_LEN] { 0 };
    size_t glassesNameSize = T5_MAX_STRING_PARAM_LEN;
    err = t5GetGlassesUtf8Param(glassesHandle, 0, kT5_ParamGlasses_UTF8_FriendlyName, glassesName, &glassesNameSize);
    if(!err)
    {
      singleUserGlassesFriendlyName = glassesName;
      std::cout << "Galsses friendly name : " << singleUserGlassesFriendlyName << std::endl;
    }

    //TODO check connection routine :
    T5_ConnectionState connectionState;
    err = t5GetGlassesConnectionState(glassesHandle, &connectionState);

    if(err) { return false; }

    err = t5ReserveGlasses(glassesHandle, GAME_NAME);
    if(err)
    {
      std::cout << "Failed to reserve glasses : " << t5GetResultMessage(err) << std::endl;
      return false;
    }

    for(;;)
    {
      err = t5EnsureGlassesReady(glassesHandle);
      if(T5_ERROR_TRY_AGAIN == err) { continue; }

      break;
    }
  }

  for(auto& textureSize : eye_render_target_sizes)
  {
    textureSize.x = defaultWidth;
    textureSize.y = defaultHeight;
  }

  err = t5InitGlassesGraphicsContext(glassesHandle, kT5_GraphicsApi_GL, nullptr);

  if(err)
  {
    std::cout << "Failed to init the OpenGL Graphics Context" << t5GetResultMessage(err) << "\n";
    return false;
  }

  //TODO obtain wand stream

  uint8_t count = WAND_BUFFER_SIZE;
  T5_WandHandle wandListBuffer[WAND_BUFFER_SIZE];
  err = t5ListWandsForGlasses(glassesHandle, wandListBuffer, &count);

  if(!err)
  {
    std::cout << "Wand List " << count << std::endl;
    for(int i = 0; i < count; ++i) std::cout << "\t- " << wandListBuffer[i];

    if(count > 0)
    {
      T5_WandStreamConfig config {};
      config.enabled = true;

      err = t5ConfigureWandStreamForGlasses(glassesHandle, &config);

      if(!err)
      {
        hand_controllers[0] = new vr_controller;
        auto& controller    = *hand_controllers;

        controller->button_names = { "t5", "1", "2", "3", "A", "B", "X", "Y" };
        //controller->buttons.resize(controller->button_names.size());

        controller->trigger_names = { "trigger" };
        //controller->triggers.resize(controller->trigger_names.size());
        //controller->stick_names   = { "" };
      }
    }
  }

  return true;
}

void vr_system_t5::build_camera_node_system()
{

  gameboard_frame = vr_tracking_anchor->push_child(create_node("T5"));
  //gameboard_frame->local_xform.scale(glm::vec3(5));
  //gameboard_frame->local_xform.rotate(-90.f, transform::X_AXIS); //Z is up for T5, Y is up for us.

  //TODO pre-scale the Gameboard Frame?
  head_node = gameboard_frame->push_child(create_node("head_node"));

  eye_camera_node[0] = head_node->push_child(create_node("eye_0"));
  eye_camera_node[1] = head_node->push_child(create_node("eye_1"));

  //We have glasses here so we can read the IPD
  double ipd        = 0;
  const auto ipderr = t5GetGlassesFloatParam(glassesHandle, 0, kT5_ParamGlasses_Float_IPD, &ipd);
  if(!ipderr)
  {
    std::cout << "TiltFive User IPD setting is " << ipd << "mm\n";
    float lhipd = -0.001 * ipd / 2.0;
    float rhipd = 0.001 * (ipd / 2.0);

    eye_camera_node[0]->local_xform.translate({ lhipd, 0, 0 });
    eye_camera_node[1]->local_xform.translate({ rhipd, 0, 0 });
  }

  {
    camera l, r;

    l.vr_eye_projection_callback = eye_projection;
    r.vr_eye_projection_callback = eye_projection;
    l.projection_type            = camera::projection_mode::vr_eye_projection;
    r.projection_type            = camera::projection_mode::vr_eye_projection;
    l.far_clip                   = 300;
    r.far_clip                   = 300;

    eye_camera[0] = eye_camera_node[0]->assign(std::move(l));
    eye_camera[1] = eye_camera_node[1]->assign(std::move(r));
  }

  hand_node[0] = gameboard_frame->push_child(create_node("wand_0"));
  if(hand_controllers[0]) hand_controllers[0]->pose_node = hand_node[0];
}

void vr_system_t5::wait_until_next_frame()
{
  //T5 no throttling or sync?

  //TODO handle device events here. We should be able to check glasses and wand connections here and assure we have exclusive mode access for rendering.
}

void vr_system_t5::update_tracking()
{
  T5_Result err = t5GetGlassesPose(glassesHandle, kT5_GlassesPoseUsage_GlassesPresentation, &pose);

  if(err)
  {
    if(T5_ERROR_TRY_AGAIN != err) std::cout << __FUNCTION__ << " err != 0 : " << t5GetResultMessage(err) << std::endl;
    if(poseIsUsable) std::cout << "T5 lost tracking.\n";
    poseIsUsable = false;
    return;
  }

  if(!poseIsUsable) std::cout << "T5 found tracking.\n";
  poseIsUsable = true;

  std::cout << pose << std::endl;

  //TODO this is just to get started, have not check tracking space.
  //TODO we might want to scale up/down the VR node anchor, as this is a tabletop experience, not a fully immersive VR one
  head_node->local_xform.set_position(T5_to_GL(pose.posGLS_GBD));
  head_node->local_xform.set_orientation(T5_to_GL(pose.rotToGLS_GBD));

  //read wand event buffer
  //Note: we probably should do it in a background thread, as the read function is blocking!
  T5_WandStreamEvent event;
  for(int i = 0; i < 2; ++i)
  {
    err = t5ReadWandStreamForGlasses(glassesHandle, &event, 1);
    if(!err)
    {
      switch(event.type)
      {
        case kT5_WandStreamEventType_Connect: break;
        case kT5_WandStreamEventType_Disconnect: break;
        case kT5_WandStreamEventType_Desync: break;
        case kT5_WandStreamEventType_Report:
          if(event.report.poseValid)
          {
            if(hand_node[0])
            {
              hand_node[0]->local_xform.set_position(T5_to_GL(event.report.posGrip_GBD));
              hand_node[0]->local_xform.set_orientation(T5_to_GL(event.report.rotToWND_GBD));
            }
          }

          detect_events(event.report);
          memcpy(&lastReport, &event.report, sizeof(T5_WandReport));

          break;
      }
    }
  }

  //TODO move this out of the VR system child, every subclass is copy pasting this snippet around...
  //This updates the world matrices on everybody
  vr_tracking_anchor->update_world_matrix();

  //This is required to update the view matrix used for rendering on our cameras
  //The camera object is not aware it's attached to a node, it only know it has a view matrix.
  //Set world matrix on a camera update the view matrix by computing the inverse of the input
  for(size_t i = 0; i < 2; ++i) eye_camera[i]->set_world_matrix(eye_camera_node[i]->get_world_matrix());
}

inline T5_Vec3 TranslateByHalfIPD(const float halfIPD, const T5_Quat& headRot, const T5_Vec3& position)
{
  glm::quat rot;
  rot.x = headRot.x;
  rot.y = headRot.y;
  rot.z = headRot.z;
  rot.w = headRot.w;

  glm::vec3 pos { position.x, position.y, position.z };
  glm::vec3 ipdOffset { halfIPD, 0, 0 };

  const auto res = pos + rot * ipdOffset;
  return { res.x, res.y, res.z };
}

void vr_system_t5::submit_frame_to_vr_system()
{
  //do not submit to T5 if the pose is nonsense, as it generates async errors to be spewed all over the output console
  if(!poseIsUsable) return;

  opengl_debug_group group(__FUNCTION__);
  T5_FrameInfo frameInfo {};
  frameInfo.leftTexHandle  = (void*)eye_render_texture[0];
  frameInfo.rightTexHandle = (void*)eye_render_texture[1];
  frameInfo.isUpsideDown   = false; //TODO maybe true. *Commiting OpenGL Crimes*
  frameInfo.isSrgb         = false;

  //TODO this is copy pasted from Pierre's example.
  frameInfo.vci.startY_VCI = -tan(glm::radians(defaultFOV) * 0.5f);
  frameInfo.vci.startX_VCI = frameInfo.vci.startY_VCI * (float)defaultWidth / (float)defaultHeight;
  frameInfo.vci.width_VCI  = -2.0f * frameInfo.vci.startX_VCI;
  frameInfo.vci.height_VCI = -2.0f * frameInfo.vci.startY_VCI;
  frameInfo.texWidth_PIX   = defaultWidth;
  frameInfo.texHeight_PIX  = defaultHeight;

  //We gonna just report the virtual camera to be exactly in the pose we received, but we gonna offset them for stereo rendering to line up
  frameInfo.posLVC_GBD   = TranslateByHalfIPD(-ipd / 2, pose.rotToGLS_GBD, pose.posGLS_GBD);
  frameInfo.posRVC_GBD   = TranslateByHalfIPD(ipd / 2, pose.rotToGLS_GBD, pose.posGLS_GBD);
  frameInfo.rotToLVC_GBD = pose.rotToGLS_GBD;
  frameInfo.rotToRVC_GBD = pose.rotToGLS_GBD;

  T5_Result err = t5SendFrameToGlasses(glassesHandle, &frameInfo);
  if(err) std::cout << t5GetResultMessage(err) << std::endl;

  //manually handle dirty GL state left by `t5SendFrameToGlasses`
  shader::force_rebind();
}

bool vr_system_t5::must_vflip() const { return false; }

void vr_system_t5::update_mr_camera() { return; }
