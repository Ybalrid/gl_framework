#pragma once

#include "vr_system.hpp"

#if USING_TILT_FIVE
#include "include/TiltFiveNative.h"

class vr_system_t5 : public vr_system
{
  node* gameboard_frame = nullptr;

  T5_Context t5ctx{};
  T5_ClientInfo clientInfo = { "info.ybalrid.todogengine-" GAME_NAME, "0.1.0", 0, 0 };
  T5_GameboardSize gameboardSizeXERaised {}, gameboardSizeXE {}, gameboardSizeLE {};
  char serviceVersion[T5_MAX_STRING_PARAM_LEN]{};
  //NOTE this only support the *one* T5 pair of glasses.
  T5_Glasses glassesHandle{};

  std::vector<std::string> glassesSerialNumber {};
  std::string singleUserGlassesSN{};
  std::string singleUserGlassesFriendlyName {};

  node* eye_camera_node[2]{nullptr, nullptr};
  T5_GlassesPose pose;
  bool poseIsUsable = false;

  static glm::vec3 T5_to_GL(const T5_Vec3& pos);
  static glm::quat T5_to_GL(const T5_Quat& rot);

  double ipd = 0;
  T5_WandReport lastReport;

  void detect_events(const T5_WandReport& currentFrameReport);

  public:
  vr_system_t5();
  virtual ~vr_system_t5();

  bool initialize(sdl::Window& window) final;
  void build_camera_node_system() final;
  void wait_until_next_frame() final;
  void update_tracking() final;
  void submit_frame_to_vr_system() final;
  [[nodiscard]] bool must_vflip() const final;
  void update_mr_camera() final;
};
#endif
