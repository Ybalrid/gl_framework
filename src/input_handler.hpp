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

	input_command* keypress(SDL_Scancode code, uint16_t mod);
	input_command* keyrelease(SDL_Scancode code, uint16_t mod);
	input_command* keyany(SDL_Scancode code, uint16_t mod);
	input_command* mouse_motion(sdl::Vec2i motion);

	std::array<keyboard_input_command*, SDL_Scancode::SDL_NUM_SCANCODES> keypress_commands, keyrelease_commands, keyany_commands;
	mouse_input_command* mouse_motion_command;

public:
	input_command* process_input_event(const sdl::Event& e);
	void setup_imgui();
	input_handler();

	void register_keypress(SDL_Scancode code, keyboard_input_command* command);
	void register_keyrelease(SDL_Scancode code, keyboard_input_command* command);
	void register_keyany(SDL_Scancode code, keyboard_input_command* command);
	void register_mouse_motion_command(mouse_input_command* command);
};
