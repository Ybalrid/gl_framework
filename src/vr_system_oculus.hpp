#pragma once
#include "vr_system.hpp"
#if USING_OCULUS_VR

#include "OVR_CAPI.h"
#include "OVR_CAPI_GL.h"
#include "Extras/OVR_CAPI_Util.h"
#include "Extras/OVR_Math.h"

class vr_system_oculus : public vr_system
{

  ovrSession session;
  ovrGraphicsLuid luid;
  ovrHmdDesc hmdDesc;
  ovrSessionStatus session_status;
  ovrSizei texture_sizes[2];
  float resolution_scale = 1.f;
  ovrTextureSwapChain swapchains[2];
  ovrEyeRenderDesc eyes[2];
  static ovrLayerEyeFov layer;

  node* eye_camera_node[2] = {nullptr, nullptr};

  long long frame_counter = 0;
  ovrTrackingState ts;
  double display_time;
  ovrPosef eye_to_hmd_pose[2];

  int current_index[2] = {0, 0};

  static void left_eye_oculus_projection(glm::mat4&, float, float);
  static void right_eye_oculus_projection(glm::mat4&, float, float);
 

  public:

  vr_system_oculus() = default;
  virtual ~vr_system_oculus();
  bool initialize() final;
  void build_camera_node_system() final;
  void wait_until_next_frame() final;
  void update_tracking() final;
  void submit_frame_to_vr_system() final;
  [[nodiscard]] bool must_vflip() const final { return false; }
};

#endif