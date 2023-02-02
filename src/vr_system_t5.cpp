#include "vr_system_t5.hpp"

static constexpr size_t GLASSES_BUFFER_SIZE { 1024 };
static constexpr size_t PARAM_BUFFER_SIZE { 1024 };

//TODO handle t5 projection
void eye_projection(glm::mat4& p,float,float) { p = glm::mat4(1); }

vr_system_t5::vr_system_t5()
{
  std::cout << "Initialized TiltFive based vr_system implementation\n";

  caps = caps_hmd_3dof | caps_hmd_6dof | caps_hand_controllers;
}

vr_system_t5::~vr_system_t5() { }

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
      std::cout << "Failed to reserve glasse : " << t5GetResultMessage(err) << std::endl;
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
    textureSize.x = 1280;
    textureSize.y = 720;
  }

  glGenTextures(1, &t5Buffer);
  glBindTexture(GL_TEXTURE_2D, t5Buffer);
  glTexImage2D(GL_TEXTURE_2D, 1, GL_RGBA8, 1280 * 2, 720, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
  glBindTexture(GL_TEXTURE_2D, 0);

  err = t5InitGlassesGraphicsContext(glassesHandle, kT5_GraphicsApi_GL, (void*)t5Buffer);

  return true;
}

void vr_system_t5::build_camera_node_system()
{
  head_node = vr_tracking_anchor->push_child(create_node("head_node"));
  eye_camera_node[0] = head_node->push_child(create_node("eye_0"));
  eye_camera_node[1] = head_node->push_child(create_node("eye_1"));

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
    eye_camera[0]                = eye_camera_node[0]->assign(std::move(l));
    eye_camera[1]                = eye_camera_node[1]->assign(std::move(r));
  }
}

void vr_system_t5::wait_until_next_frame()
{
  //T5 no throttling or sync?
}

void vr_system_t5::update_tracking()
{
  T5_GlassesPose pose;
  T5_Result err = t5GetGlassesPose(glassesHandle, kT5_GlassesPoseUsage_GlassesPresentation, &pose);

  if(err)
  {
    if(T5_ERROR_TRY_AGAIN == err) return;
  }

  //TODO this is just to get started, have not check tracking space.
  //TODO we might want to scale up/down the VR node anchor, as this is a tabletop experience, not a fully immersive VR one
  head_node->local_xform.set_position(*(glm::vec3*)&pose.posGLS_GBD.x);
  head_node->local_xform.set_orientation(*(glm::vec3*)&pose.rotToGLS_GBD.x);


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
  opengl_debug_group group(__FUNCTION__);
  T5_FrameInfo frameInfo {};
  frameInfo.leftTexHandle = (void*)eye_render_texture[0];
  frameInfo.rightTexHandle = (void*)eye_render_texture[1];
  frameInfo.isUpsideDown   = false; //TODO maybe true
  frameInfo.isSrgb         = false;
  frameInfo.texHeight_PIX  = 720;
  frameInfo.texWidth_PIX   = 1280;
  //TODO "not sure what VC (virtual camera) is supposed to be.
  frameInfo.vci.height_VCI = 720;
  frameInfo.vci.width_VCI = 1280;
  frameInfo.rotToLVC_GBD.w = 1;
  frameInfo.rotToRVC_GBD.w = 1;

  //TODO this call is pushing error messages in the console about invalid frames.
  T5_Result err = t5SendFrameToGlasses(glassesHandle, &frameInfo);
  if(err) 
  {
    std::cout << t5GetResultMessage(err) << std::endl;
  }

  //handle dirty state
  shader::force_rebind();
}

bool vr_system_t5::must_vflip() const { return false; }

void vr_system_t5::update_mr_camera() { return; }
