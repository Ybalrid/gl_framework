#include "input_handler.hpp"
#include <iostream>

#include "nameof.hpp"

input_command* input_handler::keypress(SDL_Scancode code, uint16_t modifier)
{
	if(keypress_commands[code])
		keypress_commands[code]->modifier = modifier;
	return keypress_commands[code];
}

input_command* input_handler::keyrelease(SDL_Scancode code, uint16_t modifier)
{
	if(keyrelease_commands[code])
		keyrelease_commands[code]->modifier = modifier;
	return keyrelease_commands[code];
}

input_command* input_handler::keyany(SDL_Scancode code, uint16_t modifier)
{
	if(keyany_commands[code])
		keyany_commands[code]->modifier = modifier;
	return keyany_commands[code];
}

input_command* input_handler::mouse_motion(sdl::Vec2i motion)
{
	if(mouse_motion_command)
		mouse_motion_command->motion = motion;
	return mouse_motion_command;
}

input_command* input_handler::process_input_event(const sdl::Event& e)
{
	switch(e.type)
	{
		case SDL_KEYDOWN:
			if(e.key.repeat || imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput))
				break;
			if(auto command = keypress(e.key.keysym.scancode, e.key.keysym.mod); command)
				return command;
			return keyany(e.key.keysym.scancode, e.key.keysym.mod);

		case SDL_KEYUP:
			if(e.key.repeat || imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput))
				break;
			if(auto command = keyrelease(e.key.keysym.scancode, e.key.keysym.mod); command)
				return command;
			return keyany(e.key.keysym.scancode, e.key.keysym.mod);
		case SDL_MOUSEMOTION:
			if(imgui && imgui->WantCaptureMouse)
				break;
			return mouse_motion({ e.motion.xrel, e.motion.yrel });
			break;
		case SDL_MOUSEBUTTONDOWN:
			if(imgui && imgui->WantCaptureMouse)
				break;
			break;
		case SDL_MOUSEBUTTONUP:
			if(imgui && imgui->WantCaptureMouse)
				break;
			break;
		case SDL_CONTROLLERAXISMOTION:
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			break;
		case SDL_CONTROLLERBUTTONUP:
			break;
		default:
			break;
	}

	return nullptr;
}

input_handler::input_handler() :
 controllers(sdl::GameController::open_all_available_controllers()),
 keypress_commands { nullptr },
 keyrelease_commands { nullptr },
 keyany_commands { nullptr }
{
}

void input_handler::register_keypress(SDL_Scancode code, keyboard_input_command* command)
{
	assert(code < SDL_NUM_SCANCODES);
	keypress_commands[code] = command;
}

void input_handler::register_keyrelease(SDL_Scancode code, keyboard_input_command* command)
{
	assert(code < SDL_NUM_SCANCODES);
	keyrelease_commands[code] = command;
}

void input_handler::register_keyany(SDL_Scancode code, keyboard_input_command* command)
{
	assert(code < SDL_NUM_SCANCODES);
	keyany_commands[code] = command;
}

void input_handler::register_mouse_motion_command(mouse_input_command* command)
{
	mouse_motion_command = command;
}

void input_handler::setup_imgui()
{
	imgui = &ImGui::GetIO();
}
