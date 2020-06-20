#include <cpp-sdl2/sdl.hpp>
#include <iostream>
#include "vr_system_openvr.hpp"
#include <GL/glew.h>
#include <glm/gtx/matrix_decompose.hpp>
#include <nameof.hpp>

#if USING_OPENVR
#include <openvr_capi.h>
vr::IVRSystem* static_access_hmd = nullptr;

inline glm::mat4 get_mat4_from_34(const vr::HmdMatrix34_t& mat)
{
 // clang-format off
  return { mat.m[0][0], mat.m[0][1], mat.m[0][2], mat.m[0][3],
           mat.m[1][0], mat.m[1][1], mat.m[1][2], mat.m[1][3],
           mat.m[2][0], mat.m[2][1], mat.m[2][2], mat.m[2][3],
           0.0f,        0.0f,        0.0f,        1.0f };
  // clang-format on
}

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
  if(!hmd)
  {
    sdl::show_message_box(SDL_MESSAGEBOX_ERROR, "Error: Cannot start OpenVR", vr::VR_GetVRInitErrorAsEnglishDescription(error));
    return false;
  }

  //Init of OpenVR successful
  char buffer[512];
  vr::ETrackedPropertyError prop_error;
  hmd->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_TrackingSystemName_String, buffer, 512, &prop_error);
  std::cout << "System Name  : " << buffer << "\n";
  hmd->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ManufacturerName_String, buffer, 512, &prop_error);
  std::cout << "Manufacturer : " << buffer << "\n";
  hmd->GetStringTrackedDeviceProperty(vr::k_unTrackedDeviceIndex_Hmd, vr::Prop_ModelNumber_String, buffer, 512, &prop_error);
  std::cout << "Model Number : " << buffer << "\n";

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
  glm::vec4 p;
  glm::vec3 tr, sc, sk;
  glm::quat rot;

  head_node = vr_tracking_anchor->push_child(create_node());
  for(size_t eye = 0; eye < 2; ++eye)
  {
    eye_camera_node[eye] = head_node->push_child(create_node());
    camera cam;
    eye_camera[eye] = eye_camera_node[eye]->assign(std::move(cam));
    hand_node[eye]  = head_node->push_child(create_node());

    const auto eye_matrix_34      = hmd->GetEyeToHeadTransform(vr::EVREye(eye));
    const auto eye_full_transform = (glm::transpose(get_mat4_from_34(eye_matrix_34)));
    glm::decompose(eye_full_transform, sc, rot, tr, sk, p);
    eye_camera_node[eye]->local_xform.set_position(tr);
    eye_camera_node[eye]->local_xform.set_orientation(rot);
    eye_camera[eye]->projection_type            = camera::projection_mode::vr_eye_projection;
    texture_handlers[eye].handle      = reinterpret_cast<void*>(static_cast<unsigned long long>(eye_render_texture[eye]));
    texture_handlers[eye].eType       = vr::TextureType_OpenGL;
    texture_handlers[eye].eColorSpace = vr::ColorSpace_Auto;
  }

  eye_camera[0]->vr_eye_projection_callback = &get_left_eye_proj_matrix;
  eye_camera[1]->vr_eye_projection_callback = &get_right_eye_proj_matrix;
}

void vr_system_openvr::get_left_eye_proj_matrix(glm::mat4& matrix, float near_clip, float far_clip)
{
  float left, right, top, bottom;
  static_access_hmd->GetProjectionRaw(vr::Eye_Left, &left, &right, &top, &bottom);
  matrix = glm::frustum(near_clip * left, near_clip * right, near_clip * -bottom, near_clip * -top, near_clip, far_clip);
}

void vr_system_openvr::get_right_eye_proj_matrix(glm::mat4& matrix, float near_clip, float far_clip)
{
  float left, right, top, bottom;
  static_access_hmd->GetProjectionRaw(vr::Eye_Right, &left, &right, &top, &bottom);
  matrix = glm::frustum(near_clip * left, near_clip * right, near_clip * -bottom, near_clip * -top, near_clip, far_clip);
}

vr::TrackedDeviceIndex_t find_liv_tracker()
{
  struct device
  {
    TrackedDeviceIndex_t device_index;
    int32_t device_weight;
  };

  std::vector<device> list;
  list.reserve((size_t)vr::k_unTrackedDeviceIndex_Hmd);

  //For each OpenVR Device:
  for(TrackedDeviceIndex_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
  {
    //If the device isn't connected, skip the device.
    if(!static_access_hmd->IsTrackedDeviceConnected(i)) continue;

    //If the device's "class" isn't `Controller` or `GenericTracker`, skip the device.
    const auto device_class = static_access_hmd->GetTrackedDeviceClass(i);
    if(!(device_class == ETrackedDeviceClass_TrackedDeviceClass_Controller
         || device_class == ETrackedDeviceClass_TrackedDeviceClass_GenericTracker))
      continue;

    char buff[512];
    if(static_access_hmd->GetStringTrackedDeviceProperty(i, vr::Prop_ModelNumber_String, buff, 512) > 0)
    {
      //If the device's "ModelNumber" is "LIV Virtual Camera", assign weight 10.
      if(strcmp("LIV Virtual Camera", buff) == 0)
      {
        list.push_back({ i, 10 });
        continue;
      }

      //If the device's "ModelNumber" is "Virtual Controller Device", assign weight 9.
      if(strcmp("Virtual Controller Device", buff) == 0)
      {
        list.push_back({ i, 9 });
        continue;
      }
    }

    //If the device's "class" is `GenericTracker`, assign weight 5.
    if(device_class == ETrackedDeviceClass_TrackedDeviceClass_GenericTracker)
    {
      list.push_back({ i, 5 });
      continue;
    }

    //If the device's "class" is `Controller`:
    if(device_class == ETrackedDeviceClass_TrackedDeviceClass_Controller)
    {
      //If the device's "RenderModel" is "{htc}vr_tracker_vive_1_0", assign weight 8.
      if(static_access_hmd->GetStringTrackedDeviceProperty(i, vr::Prop_RenderModelName_String, buff, 512) > 0)
      {
        if(strstr(buff, "vr_tracker_vive_1_0") != NULL)
        {
          list.push_back({ i, 8 });
          continue;
        }
      }

      //If the device's "role" is `OptOut`, assign weight 7.
      if(static_access_hmd->GetControllerRoleForTrackedDeviceIndex(i) == vr::TrackedControllerRole_OptOut)
      {
        list.push_back({ i, 7 });
        continue;
      }

      //If the device's "role" is `Invalid`, assign weight 6.
      if(static_access_hmd->GetControllerRoleForTrackedDeviceIndex(i) == vr::TrackedControllerRole_OptOut)
      {
        list.push_back({ i, 6 });
        continue;
      }
    }

    //Otherwise, assign weight 1.
    list.push_back({ i, 1 });
  }

  //From the OpenVR Devices with weight, take the device with the highest weight.
  if(!list.empty())
  {
    std::sort(list.begin(), list.end(), [](const device& a, const device& b) { return a.device_weight > b.device_weight; });
    return list.front().device_index;
  }

  return vr::k_unTrackedDeviceIndexInvalid;
}

void vr_system_openvr::update_mr_camera()
{
  static const auto liv_tracker_index = find_liv_tracker();
  if(tracked_device_pose_array[liv_tracker_index].bPoseIsValid)
  {
    const auto [translation, rotation]
        = get_translation_rotation(glm::transpose(get_mat4_from_34(tracked_device_pose_array[liv_tracker_index].mDeviceToAbsoluteTracking)));
    mr_camera_node->local_xform.set_position(translation);
    mr_camera_node->local_xform.set_orientation(rotation);
    vr_tracking_anchor->update_world_matrix();
    mr_camera->set_world_matrix(mr_camera_node->get_world_matrix());
    mr_camera->fov = 60; //TODO set this dynamically
  }
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

  //update node local transforms with OpenVR tracked objects
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
