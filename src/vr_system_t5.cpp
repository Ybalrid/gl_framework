#include "vr_system_t5.hpp"

#include "include/TiltFiveNative.hpp"

static constexpr size_t GLASSES_BUFFER_SIZE { 1024 };
static constexpr size_t PARAM_BUFFER_SIZE { 1024 };

static constexpr int defaultWidth  = 1216;
static constexpr int defaultHeight = 768;
static float defaultFOV            = 48.0;

//TODO handle t5 projection "correctly"
void eye_projection(glm::mat4& p,float nearz,float farz)
{
  p = glm::perspectiveFov<float>(glm::radians(defaultFOV), defaultWidth, defaultHeight, nearz, farz);
}

glm::vec3 vr_system_t5::T5_to_GL(const T5_Vec3& pos)
{
  glm::vec3 glPos_GBD { pos.x, pos.z, -pos.y };

  return glPos_GBD;
}

inline std::string to_string(const glm::vec3& v)
{
  return "(" + std::to_string(v.x) + ", " + std::to_string(v.y) + ", " + std::to_string(v.z) + ")";
}

glm::quat vr_system_t5::T5_to_GL(const T5_Quat& rot)
{
  glm::quat glRot_gbd;
  glRot_gbd.w = rot.w;
  glRot_gbd.x = rot.x;
  glRot_gbd.y = rot.y;
  glRot_gbd.z = rot.z;

  const auto euler = glm::eulerAngles(glRot_gbd);

  glm::vec3 flippedAngles(-euler.x, -euler.z, euler.y);
  std::cout << to_string(flippedAngles) << std::endl;

  return flippedAngles;
}

T5_Vec3 vr_system_t5::GL_to_T5(const glm::vec3& pos)
{
  T5_Vec3 t5Pos { pos.x, pos.z, -pos.y };
  return t5Pos;
}

T5_Quat vr_system_t5::GL_to_T5(const glm::quat& rot)
{
  T5_Quat t5Rot { rot.w,-rot.x, rot.y, rot.z };
  return t5Rot;
}

vr_system_t5::vr_system_t5()
{
  std::cout << "Initialized TiltFive based vr_system implementation\n";

  caps = caps_hmd_3dof | caps_hmd_6dof /* | caps_hand_controllers */;
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

    if(glassesSerialNumber.empty()) return false;
    std::cout << "found " << glassesSerialNumber.size() << " TiltFive glasses:\n";
    for(const auto& glassesName : glassesSerialNumber)
    {
      std::cout << "\t- " << glassesName << "\n";
    }


    singleUserGlassesSN = glassesSerialNumber.front();

    err = t5CreateGlasses(t5ctx, singleUserGlassesSN.c_str(), &glassesHandle);
    if(err) { std::cout << "Error creating glasses " << singleUserGlassesSN << ": " << t5GetResultMessage(err);
    }

    //TODO check connection routine :
    T5_ConnectionState connectionState;
    err = t5GetGlassesConnectionState(glassesHandle, &connectionState);

    if(err)
    {
      return false;
    }


    err = t5ReserveGlasses(glassesHandle, GAME_NAME);
    if(err)
    {
      std::cout << "Failed to reserve glasses : " << t5GetResultMessage(err) << std::endl;
      return false;
    }

    for(;;)
    { err = t5EnsureGlassesReady(glassesHandle);
      if(T5_ERROR_TRY_AGAIN == err)
      {
        continue;
      }

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
    std::cout << "Failed to init the OpenGL Graphics Context"<< t5GetResultMessage(err) <<"\n";
    return false;
  }

  return true;
}

void vr_system_t5::build_camera_node_system()
{

  gameboard_frame = vr_tracking_anchor->push_child(create_node("T5"));
  gameboard_frame->local_xform.scale(glm::vec3(2.5));
  //gameboard_frame->local_xform.rotate(-90.f, transform::X_AXIS); //Z is up for T5, Y is up for us.

  //TODO pre-scale the Gameboard Frame?
  head_node = gameboard_frame->push_child(create_node("head_node"));
  head_frame = head_node->push_child(create_node("head_frame"));
  //head_frame->local_xform.rotate(-90, transform::X_AXIS);

  eye_camera_node[0] = head_frame->push_child(create_node("eye_0"));
  eye_camera_node[1] = head_frame->push_child(create_node("eye_1"));

  eye_frame[0] = eye_camera_node[0]->push_child(create_node("eye_0_camera"));
  eye_frame[1] = eye_camera_node[1]->push_child(create_node("eye_1_camera"));

  //for(auto* eye_camera_frame : eye_frame) eye_camera_frame->local_xform.rotate(-90, transform::X_AXIS);

  //We have glasses here so we can read the IPD
  double ipd = 0;
  const auto ipderr = t5GetGlassesFloatParam(glassesHandle, 0, kT5_ParamGlasses_Float_IPD, &ipd) ;
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
    eye_camera[0]                = eye_frame[0]->assign(std::move(l));
    eye_camera[1]                = eye_frame[1]->assign(std::move(r));
  }
}

void vr_system_t5::wait_until_next_frame()
{
  //T5 no throttling or sync?
}

void vr_system_t5::update_tracking()
{
  T5_Result err = t5GetGlassesPose(glassesHandle, kT5_GlassesPoseUsage_GlassesPresentation, &pose);
  poseIsUsable = !err;

  if(err)
  {
    std::cout << __FUNCTION__ << " err != 0 : " << t5GetResultMessage(err) << std::endl;
    return;
  }

  //std::cout << pose << std::endl;

  //TODO this is just to get started, have not check tracking space.
  //TODO we might want to scale up/down the VR node anchor, as this is a tabletop experience, not a fully immersive VR one
  head_node->local_xform.set_position(T5_to_GL(pose.posGLS_GBD));
  head_node->local_xform.set_orientation(T5_to_GL(pose.rotToGLS_GBD));

  //TODO move this out of the VR system child, every subclass is copy pasting this snippet around...
  //This updates the world matrices on everybody
  vr_tracking_anchor->update_world_matrix();

  //This is required to update the view matrix used for rendering on our cameras
  //The camera object is not aware it's attached to a node, it only know it has a view matrix.
  //Set world matrix on a camera update the view matrix by computing the inverse of the input
  for(size_t i = 0; i < 2; ++i) eye_camera[i]->set_world_matrix(eye_camera_node[i]->get_world_matrix());
}

void vr_system_t5::submit_frame_to_vr_system()
{
  //do not submit to T5 if the pose is nonsense, as it generates async errors to be spewed all over the output console
  if(!poseIsUsable) 
      return;

  opengl_debug_group group(__FUNCTION__);
  T5_FrameInfo frameInfo {};
  frameInfo.leftTexHandle = (void*)eye_render_texture[0];
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

  //TODO we need to get those *relative to their parent frame*
  frameInfo.posLVC_GBD = GL_to_T5(eye_camera_node[0]->local_xform.get_position());
  frameInfo.posRVC_GBD = GL_to_T5(eye_camera_node[1]->local_xform.get_position());
  //we may be messing with those rotations to get the eye cameras to be properly anligned so... this may be invalid
  frameInfo.rotToLVC_GBD = GL_to_T5(eye_camera_node[1]->local_xform.get_orientation());
  frameInfo.rotToRVC_GBD = GL_to_T5(eye_camera_node[1]->local_xform.get_orientation());

  frameInfo.posLVC_GBD = pose.posGLS_GBD;
  frameInfo.posRVC_GBD   = pose.posGLS_GBD;
  frameInfo.rotToLVC_GBD = pose.rotToGLS_GBD;
  frameInfo.rotToRVC_GBD = pose.rotToGLS_GBD;

  T5_Result err = t5SendFrameToGlasses(glassesHandle, &frameInfo);
  if(err) 
    std::cout << t5GetResultMessage(err) << std::endl;

  //handle dirty state
  shader::force_rebind();
}

bool vr_system_t5::must_vflip() const { return false; }

void vr_system_t5::update_mr_camera() { return; }
