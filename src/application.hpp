#pragma once
#ifdef USING_JETLIVE
#include "jet/live/Live.hpp"
#include "jet/live/ILiveListener.hpp"
#include "nameof.hpp"
#include <iostream>
#endif

#include <glm/glm.hpp>

#include <GL/glew.h>
#define CPP_SDL2_ENABLE_OPENGL
#include <cpp-sdl2/sdl.hpp>

#include <vector>
#include <string>
#include "resource_system.hpp"
#include "freeimage_raii.hpp"

#include "gltf_loader.hpp"
#include "gui.hpp"
#include "script_system.hpp"

#include "node.hpp"
#include "scene.hpp"
#include "shader_program_manager.hpp"
#include "texture_manager.hpp"
#include "renderable_manager.hpp"
#include "audio_system.hpp"
#include "input_handler.hpp"
#include "camera_controller.hpp"
#include "vr_system.hpp"
#include "level_system.hpp"

#ifdef USING_JETLIVE
#ifdef _DEBUG
class jet_live_log_listener : public jet::ILiveListener
{
  public:
  void onLog(jet::LogSeverity severity, const std::string& message) override
  {
    std::cerr << "jet-live " << nameof::nameof_enum(severity) << " " << message << std::endl; //I'll accept the slow flush here
  }
};
#endif
#endif

class application
{

#ifdef USING_JETLIVE
#ifdef _DEBUG
  jet::Live liveInstance { std::make_unique<jet_live_log_listener>() };
#endif
#endif

  void activate_vsync() const;
  void draw_debug_ui();
  void update_timing();
  void set_opengl_attribute_configuration(bool multisampling, int samples, bool srgb_framebuffer) const;
  void initialize_glew() const;
  void install_opengl_debug_callback() const;
  void configure_and_create_window(const std::string& application_name);
  void create_opengl_context();
  void initialize_modern_opengl() const;
  void initialize_gui();
  void frame_prepare();
  void render_shadowmap();
  void render_draw_list(camera* render_camera);
  void build_draw_list_from_camera(camera* render_camera);
  void render_frame();
  void run_events();
  void setup_scene();

  freeimage free_img;
  resource_system resources;

  bool debug_ui              = false;
  uint32_t current_time      = 0;
  uint32_t last_frame_time   = 0;
  uint32_t last_second_time  = 0;
  uint32_t last_frame_delta  = 0;
  float last_frame_delta_sec = 0;
  float current_time_in_sec  = 0;
  int frames_in_current_sec  = 0;
  int fps                    = 0;
  size_t frames              = 0;

  sdl::Root root { SDL_INIT_EVERYTHING };
  sdl::Window window;
  sdl::Window::GlContext context;
  sdl::Event event {};

  gui ui;
  script_system scripts;
  gltf_loader gltf;
  scene s;
  static scene* main_scene;
  shader_program_manager shader_manager;
  texture_manager texture_mgr;
  renderable_manager renderable_mgr;

  bool running = true;

  camera* main_camera = nullptr;
  node* cam_node      = nullptr;
  directional_light sun {};
  std::array<point_light*, 4> p_lights { nullptr, nullptr, nullptr, nullptr };

  audio_system audio;
  input_handler inputs;
  std::unique_ptr<camera_controller> fps_camera_controller = nullptr;

  struct draw_operation
  {
    node* object_owner;
    renderable_handle primitive;
  };

  std::vector<draw_operation> draw_list;

  bool debug_draw_bbox           = false;
  shader_handle shadowmap_shader = shader_program_manager::invalid_shader;
  GLuint shadow_depth_fbo, shadow_depth_map;

  static constexpr unsigned int shadow_width = 1024, shadow_height = 1024;

  static glm::vec4 clear_color;

  struct keyboard_debug_utilities_
  {
    application* parent_;
    struct toggle_console_keyboard_command_ : keyboard_input_command
    {
      application* parent_;
      void execute() override;
      toggle_console_keyboard_command_(application* parent) : parent_ { parent } { }
    } toggle_console_keyboard_command { parent_ };

    struct toggle_debug_keyboard_command_ : keyboard_input_command
    {
      application* parent_;
      void execute() override;
      toggle_debug_keyboard_command_(application* parent) : parent_ { parent } {};
    } toggle_debug_keyboard_command { parent_ };

    struct toggle_live_code_reload_command_ : keyboard_input_command
    {
      application* parent_;
      void execute() override;
      toggle_live_code_reload_command_(application* parent) : parent_ { parent } {};
    } toggle_live_code_reload_command { parent_ };

    keyboard_debug_utilities_(application* parent) : parent_ { parent } { }
  } keyboard_debug_utilities { this };


  struct gamepad_button_test_command_ : gamepad_button_command
  {
    void execute() override;
  } gamepad_button_test_command{};

  vr_system_ptr vr  = nullptr;
  bool mr_activated = false;

  void splash_frame(const char* image_path = "/textures/splash.png");

  level_system levels;
  std::string start_level_name;

  public:
  static scene* get_main_scene();
  void run();
  void setup_lights();
  application(int argc, char** argv, const std::string& application_name);
  static std::vector<std::string> resource_paks;
  static void set_clear_color(glm::vec4 color);
};
