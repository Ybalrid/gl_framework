#pragma once

#include "vr_system.hpp"

#include "include/TiltFiveNative.h"

class vr_system_t5 : public vr_system
{
  T5_Context t5ctx;
  T5_ClientInfo clientInfo = { "info.ybalrid.todogengine-" GAME_NAME, "0.1.0", 0, 0 };
  T5_GameboardSize gameboardSizeXERaised {}, gameboardSizeXE {}, gameboardSizeLE {};
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
