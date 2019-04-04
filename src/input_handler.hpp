#pragma once

#include <cpp-sdl2/event.hpp>
#include <cpp-sdl2/game_controller.hpp>
#include <imgui.h>

class input_handler
{
	std::vector<sdl::GameController> controllers;
	ImGuiIO* imgui { nullptr };

public:
	void event(const sdl::Event& e);
	void setup_imgui();
	input_handler();
};
