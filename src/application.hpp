#pragma once

#ifdef USING_JETLIVE
#include "jet/live/Live.hpp"
#include "jet/live/ILiveListener.hpp"
#include "nameof.hpp"
#include <iostream>
#endif

#include <GL/glew.h>
#define CPP_SDL2_GL_WINDOW
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

//class gltf_loader;

class application
{

#ifdef USING_JETLIVE
#ifdef _DEBUG
	jet::Live liveInstance { std::make_unique<jet_live_log_listener>() };
#endif
#endif

	void activate_vsync();
	void draw_debug_ui();
	void update_timing();
	void set_opengl_attribute_configuration(bool multisampling, int samples, bool srgb_framebuffer) const;
	void initialize_glew() const;
	void install_opengl_debug_callback() const;
	void configure_and_create_window();
	void create_opengl_context();

	freeimage free_img;
	resource_system resources;

	bool debug_ui			   = false;
	uint32_t current_time	  = 0;
	uint32_t last_frame_time   = 0;
	uint32_t last_second_time  = 0;
	uint32_t last_frame_delta  = 0;
	float last_frame_delta_sec = 0;
	float current_time_in_sec  = 0;
	int frames_in_current_sec  = 0;
	int fps					   = 0;
	size_t frames			   = 0;

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
	node* cam_node		= nullptr;
	directional_light sun;
	std::array<point_light*, 4> p_lights { nullptr, nullptr, nullptr, nullptr };

	//key states
	bool up = false, down = false, left = false, right = false, mouse = false;
	float mousex = 0, mousey = 0;

	audio_system audio;
	input_handler inputs;

public:
	static scene* get_main_scene();
	void initialize_modern_opengl();
	void initialize_gui();
	void render_frame();
	void run_events();
	void run();
	void setup_scene();
	application(int argc, char** argv);
	static std::vector<std::string> resource_paks;
};
