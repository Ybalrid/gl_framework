#include "build_config.hpp"
#if USING_OCULUS_VR
#include "vr_system_oculus.hpp"

#include <mutex>
#include <string>
#include <sstream>

ovrLayerEyeFov vr_system_oculus::layer;

struct log_ctx
{
  std::mutex mtx;
} logger_lock;

vr_system_oculus::vr_system_oculus() : vr_system()
{
  std::cout << "Initialized Oculus VR based vr_system implementation\n";

  caps = caps_hmd_3dof | caps_hmd_6dof;
}

vr_system_oculus::~vr_system_oculus()
{
  std::cout << "Deinitialized Oculus VR based vr_system implementation\n";

  ovr_Destroy(session);
  ovr_Shutdown();
}

bool vr_system_oculus::initialize(sdl::Window& window)
{
  ovrInitParams init_params = {};
  init_params.Flags         = 0;
  //init_params.Flags         = ovrInit_FocusAware;
  //#if !defined(NDEBUG)
  //  init_params.Flags |= ovrInit_Debug;
  //#endif

  init_params.LogCallback = [](uintptr_t ctx_ptr, int level, const char* message) {
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
            << "\t - display     : " << hmdDesc.Resolution.w << "x" << hmdDesc.Resolution.h << " " << hmdDesc.DisplayRefreshRate
            << "Hz\n"
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
    if(OVR_FAILURE(ovr_CreateTextureSwapChainGL(session, &texture_swap_chain_desc, &swapchains[eye])))
    {
      std::cerr << "Wasn't able to create OpenGL swapchains\n";
      return false;
    }

    eyes[eye] = ovr_GetRenderDesc(session, (ovrEyeType)(eye), hmdDesc.DefaultEyeFov[eye]);
  }

  //Define one Oculus Compositor layer for the user's FoV
  layer.Header.Type     = ovrLayerType_EyeFov;
  layer.Header.Flags    = ovrLayerFlag_TextureOriginAtBottomLeft;
  layer.ColorTexture[0] = swapchains[0];
  layer.ColorTexture[1] = swapchains[1];
  layer.Fov[0]          = eyes[0].Fov;
  layer.Fov[1]          = eyes[1].Fov;
  layer.Viewport[0]     = OVR::Recti { 0, 0, texture_sizes[0].w, texture_sizes[0].h };
  layer.Viewport[1]     = OVR::Recti { 0, 0, texture_sizes[1].w, texture_sizes[1].h };

  return true;
}

void vr_system_oculus::left_eye_oculus_projection(glm::mat4& output, float near_plane, float far_plane)
{
  const auto& fov   = layer.Fov[0];
  const auto matrix = ovrMatrix4f_Projection(fov, near_plane, far_plane, ovrProjection_ClipRangeOpenGL);
  output            = glm::make_mat4x4((float*)&matrix.M[0][0]);
  output            = glm::transpose(output);
}

void vr_system_oculus::right_eye_oculus_projection(glm::mat4& output, float near_plane, float far_plane)
{
  const auto& fov   = layer.Fov[1];
  const auto matrix = ovrMatrix4f_Projection(fov, near_plane, far_plane, ovrProjection_ClipRangeOpenGL);
  output            = glm::make_mat4x4((float*)&matrix.M[0][0]);
  output            = glm::transpose(output);
}

void vr_system_oculus::build_camera_node_system()
{
  head_node = vr_tracking_anchor->push_child(create_node());
  for(size_t i = 0; i < 2; ++i)
  {
    eye_camera_node[i] = head_node->push_child(create_node());
    camera cam;
    eye_camera[i] = eye_camera_node[i]->assign(std::move(cam));
    hand_node[i]  = head_node->push_child(create_node());
  }

  eye_camera[0]->vr_eye_projection_callback = &left_eye_oculus_projection;
  eye_camera[1]->vr_eye_projection_callback = &right_eye_oculus_projection;
  eye_camera[0]->projection_type            = camera::projection_mode::vr_eye_projection;
  eye_camera[1]->projection_type            = camera::projection_mode::vr_eye_projection;
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

  eyes[0] = ovr_GetRenderDesc(session, ovrEye_Left, hmdDesc.DefaultEyeFov[0]);
  eyes[1] = ovr_GetRenderDesc(session, ovrEye_Right, hmdDesc.DefaultEyeFov[1]);

  eye_to_hmd_pose[0] = eyes[0].HmdToEyePose;
  eye_to_hmd_pose[1] = eyes[1].HmdToEyePose;
  ovr_GetEyePoses(session, frame_counter, ovrTrue, eye_to_hmd_pose, layer.RenderPose, &layer.SensorSampleTime);

  for(auto eye = 0; eye < 2; ++eye)
  {
    eye_camera_node[eye]->local_xform.set_position(glm::make_vec3((float*)&eyes[eye].HmdToEyePose.Position));
    eye_camera_node[eye]->local_xform.set_orientation(glm::make_quat((float*)&eyes[eye].HmdToEyePose.Orientation));
  }

  head_node->local_xform.set_position(glm::make_vec3((float*)&ts.HeadPose.ThePose.Position));
  head_node->local_xform.set_orientation(glm::make_quat((float*)&ts.HeadPose.ThePose.Orientation));

  //update nodes world transforms
  vr_tracking_anchor->update_world_matrix();
  for(auto eye = 0; eye < 2; ++eye) eye_camera[eye]->set_world_matrix(eye_camera_node[eye]->get_world_matrix());
}

void vr_system_oculus::submit_frame_to_vr_system()
{
  for(auto eye = 0; eye < 2; ++eye)
  {
    GLuint oculus_owned_texture = -1;
    ovr_GetTextureSwapChainCurrentIndex(session, swapchains[eye], &current_index[eye]);
    ovr_GetTextureSwapChainBufferGL(session, swapchains[eye], current_index[eye], &oculus_owned_texture);
    // clang-format off
    glCopyImageSubData(eye_render_texture[eye],
                       GL_TEXTURE_2D,
                       0,0,0,0,
                       oculus_owned_texture,
                       GL_TEXTURE_2D,
                       0,0,0,0,
                       eye_render_target_sizes[eye].x,
                       eye_render_target_sizes[eye].y, 1);
    // clang-format on
    ovr_CommitTextureSwapChain(session, swapchains[eye]);
  }
  const auto* layers = &layer.Header;
  ovr_EndFrame(session, frame_counter, nullptr, &layers, 1);
  frame_counter++;
}
#endif
