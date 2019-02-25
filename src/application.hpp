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
class application
{
	bool running = true;
	static void activate_vsync();

	void handle_event(const sdl::Event& e);

	resource_system resources;

public:
	application(int argc, char** argv);
	static std::vector<std::string> resource_paks;
};
