#pragma once

#include "vr_system.hpp"

#include "include/TiltFiveNative.h"

class vr_system_t5 : public vr_system
{
  node* gameboard_frame = nullptr;
  node* eye_frame[2] { nullptr, nullptr };
  node* head_frame = nullptr;

  T5_Context t5ctx{};
  T5_ClientInfo clientInfo = { "info.ybalrid.todogengine-" GAME_NAME, "0.1.0", 0, 0 };
  T5_GameboardSize gameboardSizeXERaised {}, gameboardSizeXE {}, gameboardSizeLE {};
  char serviceVersion[T5_MAX_STRING_PARAM_LEN]{};
  //NOTE this only support the *one* T5 pair of glasses.
  T5_Glasses glassesHandle{};

  std::vector<std::string> glassesSerialNumber {};
  std::string singleUserGlassesSN{};

  node* eye_camera_node[2]{nullptr, nullptr};
  GLuint t5Buffer;
  T5_GlassesPose pose;
  bool poseIsUsable = false;

  static glm::vec3 T5_to_GL(const T5_Vec3& pos);
  static glm::quat T5_to_GL(const T5_Quat& rot);
  static T5_Vec3 GL_to_T5(const glm::vec3& pos);
  static T5_Quat GL_to_T5(const glm::quat& rot);


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
