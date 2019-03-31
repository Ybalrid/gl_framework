#pragma once

#include "input_handler.hpp"
#include <iostream>
void input_handler::event(const sdl::Event & e)
{
	using std::cout;
	switch(e.type)
	{
	case SDL_KEYDOWN:
		cout << "keydown\n";
		break;
	case SDL_KEYUP:
		cout << "keyup\n";
		break;
	case SDL_MOUSEMOTION:
		cout << "mousemotion\n";
		break;
	case SDL_MOUSEBUTTONDOWN:
		cout << "mousebuttondown\n";
		break;
	case SDL_MOUSEBUTTONUP:
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
