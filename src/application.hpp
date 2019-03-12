#pragma once

#include <GL/glew.h>
#define CPP_SDL2_GL_WINDOW
#include <cpp-sdl2/sdl.hpp>
#include <iostream>

#include <vector>
#include <string>
#include "resource_system.hpp"
#include "texture.hpp"
#include "scene_object.hpp"
#include "gltf_loader.hpp"
#include "freeimage_raii.hpp"
#include "gui.hpp"
#include "script_system.hpp"

#include "node.hpp"
#include "scene.hpp"
#include "shader_program_manager.hpp"

class application
{
	void activate_vsync();
	void handle_event(const sdl::Event& e);
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
	int frames				   = 0;
	int fps					   = 0;

	sdl::Root root{ SDL_INIT_EVERYTHING };
	sdl::Window window;
	sdl::Window::GlContext context;
	sdl::Event event{};

	gui ui;
	bool running = true;

	script_system scripts;
	gltf_loader gltf;

	scene s;
	static scene* main_scene;

	shader_program_manager shader_manager;
	texture_manager texture_mgr;

public:
	static scene* get_main_scene();
	void initialize_modern_opengl();
	void initialize_gui();
	application(int argc, char** argv);
	static std::vector<std::string> resource_paks;
};
