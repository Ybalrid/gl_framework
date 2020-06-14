#include <cpp-sdl2/sdl.hpp>
#include <iostream>
#include "vr_system_openvr.hpp"
#include <gl/glew.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <nameof.hpp>

#if USING_OPENVR

vr::IVRSystem* static_access_hmd = nullptr;

inline glm::mat4 get_mat4_from_34(const vr::HmdMatrix34_t& mat)
{
  return { mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3], mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
           mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3], 0.0f,        0.0f,        0.0f,        1.0f };
}

//TODO we need a "pose" abstraction instead of a tuple here D:
inline std::tuple<glm::vec3, glm::quat> get_translation_rotation(const glm::mat4& mat)
{
  static glm::vec3 tr, sc, sk;
  static glm::vec4 persp;
  static glm::quat rot;

  glm::decompose(mat, sc, rot, tr, sk, persp);
  return std::tie(tr, rot);
}

vr_system_openvr::vr_system_openvr() : vr_system() { std::cout << "Initialized OpenVR based vr_system implementation\n"; }

vr_system_openvr::~vr_system_openvr()
{
  std::cout << "Deinitialized OpenVR based vr_system implementation\n";

  deinitialize_openvr();

  if(vr_tracking_anchor) vr_tracking_anchor->clean_child_list();
}

bool vr_system_openvr::initialize()
{
  //Pre-init fastchecks
  if(!vr::VR_IsRuntimeInstalled())
  {
    sdl::show_message_box(SDL_MESSAGEBOX_INFORMATION,
                          "Please install SteamVR",
                          "You are attempting to use an OpenVR application "
                          "without having installed the runtime");
    return false;
  }

  std::string runtime_path;
  //char buffer[1024];

  if(!vr::VR_IsHmdPresent())
  {
    sdl::show_message_box(SDL_MESSAGEBOX_INFORMATION,
                          "Please connect an HMD",
                          "This application cannot detect an HMD being present. Will not start VR");
    return false;
  }

  //Init attempt
  vr::EVRInitError error;
  hmd = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Scene);

  //Init of OpenVR sucessfull
  if(!hmd)
  {
    sdl::show_message_box(SDL_MESSAGEBOX_ERROR, "Error: Cannot start OpenVR", VR_GetVRInitErrorAsSymbol(error));
    return false;
  }

  if(!vr::VRCompositor())
  {
    sdl::show_message_box(
        SDL_MESSAGEBOX_ERROR, "Error: OpenVR compositor is not accessible", "Please check that SteamVR is running properly.");
    return false;
  }

  for(size_t i = 0; i < 2; i++)
  {
    //Acquire textures sizes
    uint32_t w, h;
    hmd->GetRecommendedRenderTargetSize(&w, &h);
    eye_render_target_sizes[i].x = w;
    eye_render_target_sizes[i].y = h;
  }

  init_success      = true;
  static_access_hmd = hmd;

  return init_success;
}

void vr_system_openvr::deinitialize_openvr()
{
  if(init_success)
  {
    glDeleteTextures(2, eye_render_texture);
    glDeleteRenderbuffers(2, eye_render_depth);
    //just to avoid deleting the currently bound framebuffer
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(2, eye_fbo);

    for(auto& size : eye_render_target_sizes) size = { 0, 0 };

    //openvr cleanup
    vr::VR_Shutdown();
  }
}

void vr_system_openvr::wait_until_next_frame()
{
  vr::VRCompositor()->WaitGetPoses(tracked_device_pose_array, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
}

void vr_system_openvr::build_camera_node_system()
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

  glm::vec4 persp;
  glm::vec3 tr, sc, sk;
  glm::quat rot;
  const auto left_eye_matrix_34      = hmd->GetEyeToHeadTransform(vr::Eye_Left);
  const auto left_eye_full_transform = (glm::transpose(get_mat4_from_34(left_eye_matrix_34)));
  glm::decompose(left_eye_full_transform, sc, rot, tr, sk, persp);
  eye_camera_node[0]->local_xform.set_position(tr);
  eye_camera_node[0]->local_xform.set_orientation(glm::normalize(rot));

  const auto right_eye_matrix_34      = hmd->GetEyeToHeadTransform(vr::Eye_Right);
  const auto right_eye_full_transform = (glm::transpose(get_mat4_from_34(right_eye_matrix_34)));
  glm::decompose(right_eye_full_transform, sc, rot, tr, sk, persp);
  eye_camera_node[1]->local_xform.set_position(tr);
  eye_camera_node[1]->local_xform.set_orientation(glm::normalize(rot));

  eye_camera[0]->vr_eye_projection_callback = &get_left_eye_proj_matrix;
  eye_camera[1]->vr_eye_projection_callback = &get_right_eye_proj_matrix;
  eye_camera[0]->projection_type            = camera::projection_mode::eye_vr;
  eye_camera[1]->projection_type            = camera::projection_mode::eye_vr;

  for(int eye = 0; eye < 2; ++eye)
  {
    texture_handlers[eye].handle = (void*)eye_render_texture[eye];
    texture_handlers[eye].eType  = vr::TextureType_OpenGL;
    texture_handlers[eye].eColorSpace = vr::ColorSpace_Auto;
  }

}

void vr_system_openvr::get_left_eye_proj_matrix(glm::mat4& matrix, float near_clip, float far_clip)
{
  matrix = glm::transpose(glm::make_mat4((float*)static_access_hmd->GetProjectionMatrix(vr::Eye_Left, near_clip, far_clip).m));
}

void vr_system_openvr::get_right_eye_proj_matrix(glm::mat4& matrix, float near_clip, float far_clip)
{
  matrix = glm::transpose(glm::make_mat4((float*)static_access_hmd->GetProjectionMatrix(vr::Eye_Right, near_clip, far_clip).m));
}

void vr_system_openvr::update_tracking()
{
  glm::mat4 head_tracking = glm::mat4(1);
  for(vr::TrackedDeviceIndex_t device_index = 0; device_index < vr::k_unMaxTrackedDeviceCount; ++device_index)
  {
    if(tracked_device_pose_array[device_index].bPoseIsValid)
    {
      if(hmd->GetTrackedDeviceClass(device_index) == vr::TrackedDeviceClass_HMD)
      { head_tracking = glm::transpose(get_mat4_from_34(tracked_device_pose_array[device_index].mDeviceToAbsoluteTracking)); }
    }
  }

  //update node local transforms with openvr tracked objects
  const auto [translation, rotation] = get_translation_rotation(head_tracking);
  head_node->local_xform.set_position(translation);
  head_node->local_xform.set_orientation(rotation);

  //update nodes world transforms
  vr_tracking_anchor->update_world_matrix();

  for(size_t i = 0; i < 2; ++i) eye_camera[i]->set_world_matrix(eye_camera_node[i]->get_world_matrix());
}

void vr_system_openvr::submit_frame_to_vr_system()
{
  const auto left_error  = vr::VRCompositor()->Submit(vr::Eye_Left, &texture_handlers[0]);
  const auto right_error = vr::VRCompositor()->Submit(vr::Eye_Right, &texture_handlers[1]);
  glFlush();

  if(left_error != vr::VRCompositorError_None) { std::cerr << NAMEOF_ENUM(left_error) << "\n"; }
  if(right_error != vr::VRCompositorError_None) { std::cerr << NAMEOF_ENUM(right_error) << "\n"; }
}
#endif
