#pragma once

#include <cpp-sdl2/event.hpp>
#include <cpp-sdl2/game_controller.hpp>
#include <imgui.h>

#include "input_command.hpp"

#include <array>

class input_handler
{
	std::vector<sdl::GameController> controllers;
	ImGuiIO* imgui { nullptr };

	input_command* keypress(SDL_Scancode code);
	input_command* keyrelease(SDL_Scancode code);

	std::array<input_command*, SDL_Scancode::SDL_NUM_SCANCODES> keypress_commands, keyrelease_commands;

public:
	input_command* process_input_event(const sdl::Event& e);
	void setup_imgui();
	input_handler();

	void register_keypress(SDL_Scancode code, input_command* command);
	void register_keyrelease(SDL_Scancode code, input_command* command);
};
