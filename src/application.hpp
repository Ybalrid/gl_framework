#pragma once

#include <GL/glew.h>
#define CPP_SDL2_GL_WINDOW
#include <cpp-sdl2/sdl.hpp>
#include <iostream>

#include <vector>
#include <string>
#include "resource_system.hpp"
#include "texture.hpp"
#include <FreeImage.h>
#include "scene_object.hpp"
#include "gltf_loader.hpp"

class application
{
	bool running = true;
	static void activate_vsync();

	void handle_event(const sdl::Event& e);
	void draw_debug_ui();
	void animate(scene_object& plane0, scene_object& plane1, scene_object& plane2);
	void update_timing();

	resource_system resources;

	bool debug_ui = false;

	uint32_t current_time = 0, last_frame_time, last_second_time = 0;
	int frames = 0;
	int fps = 0;


public:
	application(int argc, char** argv);
	static std::vector<std::string> resource_paks;
};
