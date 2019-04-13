#pragma once

#include <cpp-sdl2/event.hpp>
#include <cpp-sdl2/game_controller.hpp>
#include <imgui.h>

#include "input_command.hpp"

class input_handler
{
	std::vector<sdl::GameController> controllers;
	ImGuiIO* imgui { nullptr };

public:
	input_command* event(const sdl::Event& e);
	void setup_imgui();
	input_handler();
};
