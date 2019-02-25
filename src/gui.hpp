#pragma once
#include <imgui.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>

#define CPP_SDL2_GL_WINDOW
#include <cpp-sdl2/sdl.hpp>

#include <GL/glew.h>

class gui
{
	SDL_Window* w = nullptr;
public:
	gui(sdl::Window& window, sdl::Window::GlContext& gl_context);
	~gui();
	void frame();
	void render();
	void handle_event(sdl::Event e);
};

