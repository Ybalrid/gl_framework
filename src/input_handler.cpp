#include "input_handler.hpp"
#include <iostream>

void input_handler::event(const sdl::Event& e)
{
	using std::cout;
	switch(e.type)
	{
		case SDL_KEYDOWN:
			if(imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput)) break;
			cout << "keydown\n";
			break;
		case SDL_KEYUP:
			if(imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput)) break;
			cout << "keyup\n";
			break;
		case SDL_MOUSEMOTION:
			if(imgui && imgui->WantCaptureMouse) break;
			cout << "mousemotion\n";
			break;
		case SDL_MOUSEBUTTONDOWN:
			if(imgui && imgui->WantCaptureMouse) break;
			cout << "mousebuttondown\n";
			break;
		case SDL_MOUSEBUTTONUP:
			if(imgui && imgui->WantCaptureMouse) break;
			cout << "mousebuttonup\n";
			break;
		case SDL_CONTROLLERAXISMOTION:
			cout << "controlleraxismotion\n";
			break;
		case SDL_CONTROLLERBUTTONDOWN:
			cout << "controllerbuttondown\n";
			break;
		case SDL_CONTROLLERBUTTONUP:
			cout << "controllerbuttonup\n";
			break;
		default:
			break;
	}
}

input_handler::input_handler() :
 controllers(sdl::GameController::open_all_available_controllers())
{
}

void input_handler::setup_imgui()
{
	imgui = &ImGui::GetIO();
}
