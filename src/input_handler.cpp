#include "input_handler.hpp"
#include <iostream>

#include "nameof.hpp"

input_command* input_handler::keypress(SDL_Scancode code)
{
	return keypress_commands[code];
}

input_command* input_handler::keyrelease(SDL_Scancode code)
{
	return keyrelease_commands[code];
}

input_command* input_handler::process_input_event(const sdl::Event& e)
{
	switch(e.type)
	{
		case SDL_KEYDOWN:
			if(e.key.repeat || imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput))
				break;
			return keypress(e.key.keysym.scancode);
		case SDL_KEYUP:
			if(e.key.repeat || imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput))
				break;
			return keyrelease(e.key.keysym.scancode);
		case SDL_MOUSEMOTION:
			if(imgui && imgui->WantCaptureMouse)
				break;
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
 controllers(sdl::GameController::open_all_available_controllers()), keypress_commands{nullptr}, keyrelease_commands{nullptr}
{
}

void input_handler::register_keypress(SDL_Scancode code, input_command* command)
{
	assert(code < SDL_NUM_SCANCODES);
	keypress_commands[code] = command;
}

void input_handler::register_keyrelease(SDL_Scancode code, input_command* command)
{
	assert(code < SDL_NUM_SCANCODES);
	keyrelease_commands[code] = command;
}

void input_handler::setup_imgui()
{
	imgui = &ImGui::GetIO();
}
