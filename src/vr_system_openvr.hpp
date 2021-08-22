#pragma once

#include "vr_system.hpp"
#if USING_OPENVR

#include "openvr.h"

class vr_system_openvr : public vr_system
{
  vr::IVRSystem* hmd                                                               = nullptr;
  vr::TrackedDevicePose_t tracked_device_pose_array[vr::k_unMaxTrackedDeviceCount] = { 0 };
  bool init_success                                                                = false;
  vr::Texture_t texture_handlers[2];

  node* eye_camera_node[2] = { nullptr, nullptr };

  public:
  vr_system_openvr();

  virtual ~vr_system_openvr();

  bool initialize(sdl::Window& window) final;

  void update_tracking() final;
  void wait_until_next_frame() final;
  void submit_frame_to_vr_system() final;
  void build_camera_node_system() final;
  [[nodiscard]] virtual bool must_vflip() const final { return false; }

  static void get_left_eye_proj_matrix(glm::mat4& matrix, float near_clip, float far_clip);
  static void get_right_eye_proj_matrix(glm::mat4& matrix, float near_clip, float far_clip);

#ifdef WIN32
  void update_mr_camera() final;
#endif
  [[nodiscard]]  renderable_handle load_controller_model_from_runtime(vr_controller::hand_side side, shader_handle shader) final;

};

#endif
