#include "build_config.hpp"
#if USING_OCULUS_VR
#include "vr_system_oculus.hpp"

#include <mutex>
#include <string>
#include <sstream>

struct log_ctx
{
  std::mutex mtx;
} logger_lock;

vr_system_oculus::~vr_system_oculus()
{
  ovr_Destroy(session);
  ovr_Shutdown();
}

bool vr_system_oculus::initialize()
{
  ovrInitParams init_params = {};
  init_params.Flags         = ovrInit_FocusAware;
#if !defined(NDEBUG)
  init_params.Flags |= ovrInit_Debug;
#endif

  init_params.LogCallback   = [](uintptr_t ctx_ptr, int level, const char* message) {
    log_ctx* ctx = (log_ctx*)ctx_ptr;
    std::lock_guard<std::mutex> stack_lock { ctx->mtx };
    std::cerr << "Oculus [" << level << "] : " << message << "\n";
  };
  init_params.UserData = (uintptr_t)&logger_lock;
  if(OVR_FAILURE(ovr_Initialize(&init_params)))
  {
    std::cerr << "ovr_Initialize failed\n";
    return false;
  }

  std::stringstream clientIdentifier;
  clientIdentifier << "EngineName: The //TODO engine\n";
  clientIdentifier << "EngineVersion: 0.0.0.0\n";
  ovr_IdentifyClient(clientIdentifier.str().c_str());

  if(OVR_FAILURE(ovr_Create(&session, &luid)))
  {
    std::cerr << "Cannot create OVR session\n";
    ovr_Shutdown();
    return false;
  }

  hmdDesc = ovr_GetHmdDesc(session);

  std::cout << "Created session for OculusVR headset:\n";
  std::cout << "\t - manufacturer: " << hmdDesc.Manufacturer << "\n"
            << "\t - product name: " << hmdDesc.ProductName << "\n"
            << "\t - display     : " << hmdDesc.Resolution.w << "x" << hmdDesc.Resolution.h << " " << hmdDesc.DisplayRefreshRate<< "Hz\n"
            << "\t - firmware    : " << hmdDesc.FirmwareMajor << "." << hmdDesc.FirmwareMinor << "\n";

  ovr_SetTrackingOriginType(session, ovrTrackingOrigin_FloorLevel);
  ovr_GetSessionStatus(session, &session_status);


  for(size_t eye = 0; eye < 2; ++eye)
  {
    texture_sizes[eye] = ovr_GetFovTextureSize(session, (ovrEyeType)eye, hmdDesc.DefaultEyeFov[eye], resolution_scale);
    eye_render_target_sizes[eye].x                  = texture_sizes[eye].w;
    eye_render_target_sizes[eye].y                  = texture_sizes[eye].h;
    ovrTextureSwapChainDesc texture_swap_chain_desc = {};
    texture_swap_chain_desc.Type                    = ovrTexture_2D;
    texture_swap_chain_desc.ArraySize               = 1;
    texture_swap_chain_desc.Format                  = OVR_FORMAT_R8G8B8A8_UNORM_SRGB;
    texture_swap_chain_desc.Width                   = texture_sizes[eye].w;
    texture_swap_chain_desc.Height                  = texture_sizes[eye].h;
    texture_swap_chain_desc.MipLevels               = 1;
    texture_swap_chain_desc.SampleCount             = 1;
    texture_swap_chain_desc.StaticImage             = ovrFalse;
    ovr_CreateTextureSwapChainGL(session, &texture_swap_chain_desc, &swapchains[eye]);

    eyes[eye] = ovr_GetRenderDesc(session, (ovrEyeType)(eye), hmdDesc.DefaultEyeFov[eye]);
  }

  //Define one Oculus Compositor layer for the user's FoV
  layer.Header.Type     = ovrLayerType_EyeFov;
  layer.Header.Flags    = 0;
  layer.ColorTexture[0] = swapchains[0];
  layer.ColorTexture[1] = swapchains[1];
  layer.Fov[0]          = eyes[0].Fov;
  layer.Fov[1]          = eyes[1].Fov;

  glGenTextures(2, eye_render_texture);
  glGenRenderbuffers(2, eye_render_depth);
  glGenFramebuffers(2, eye_fbo);

  for(size_t i = 0; i < 2; ++i)
  {
    auto w = eye_render_target_sizes[i].x;
    auto h = eye_render_target_sizes[i].y;
    //Configure textures
    glBindFramebuffer(GL_FRAMEBUFFER, eye_fbo[i]);
    glBindTexture(GL_TEXTURE_2D, eye_render_texture[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eye_render_texture[i], 0);

    glBindRenderbuffer(GL_RENDERBUFFER, eye_render_depth[i]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, eye_render_depth[i]);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    { std::cerr << "eye fbo " << i << " is not complete" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << "\n"; }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  return true;
}

void vr_system_oculus::build_camera_node_system()
{
  head_node = vr_tracking_anchor->push_child(create_node());
  for(size_t i = 0; i < 2; ++i)
  {
    eye_camera_node[i] = head_node->push_child(create_node());
    camera cam;
    //TODO setup custom projection matrix callback
    eye_camera[i] = eye_camera_node[i]->assign(std::move(cam));

    hand_node[i] = head_node->push_child(create_node());
  }
}

void vr_system_oculus::wait_until_next_frame()
{
  ovr_WaitToBeginFrame(session, frame_counter);
  ovr_BeginFrame(session, frame_counter);
}

void vr_system_oculus::update_tracking()
{
  display_time = ovr_GetPredictedDisplayTime(session, frame_counter);
  ts           = ovr_GetTrackingState(session, display_time, ovrTrue);

  ovr_CalcEyePoses(ts.HeadPose.ThePose, eye_to_hmd_pose, layer.RenderPose);
  for(auto eye = 0; eye < 2; ++eye)
  {
    eye_camera_node[eye]->local_xform.set_position(glm::make_vec3((float*)&eyes[eye].HmdToEyePose.Position));
    eye_camera_node[eye]->local_xform.set_orientation(glm::make_quat((float*)&eyes[eye].HmdToEyePose.Orientation));
  }

  head_node->local_xform.set_position(glm::make_vec3((float*)&ts.HeadPose.ThePose.Position));
  head_node->local_xform.set_orientation(glm::make_quat((float*)&ts.HeadPose.ThePose.Orientation));

}

void vr_system_oculus::submit_frame_to_vr_system()
{
  ovr_GetSessionStatus(session, &session_status);
  int index = -1;
  GLuint oculus_owned_texture = -1;
  for(auto eye = 0; eye < 2; ++eye) 
  {
    //ovr_GetTextureSwapChainCurrentIndex(session, swapchains[eye], &index);
    ovr_GetTextureSwapChainBufferGL(session, swapchains[eye], index, &oculus_owned_texture);
    glCopyImageSubData(eye_render_texture[eye],
                       GL_TEXTURE_2D,
                       0,
                       0,
                       0,
                       0,
                       oculus_owned_texture,
                       GL_TEXTURE_2D,
                       0,
                       0,
                       0,
                       0,
                       eye_render_target_sizes[eye].x,
                       eye_render_target_sizes[eye].y,
                       1);
    ovr_CommitTextureSwapChain(session, swapchains[eye]);
  }
  auto layers = &layer.Header;
  ovr_EndFrame(session, frame_counter, nullptr, &layers, 1);
  frame_counter++;
}

#endif
