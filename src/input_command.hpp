#pragma once

#include <cpp-sdl2/vec2.hpp>

struct input_command
{
	virtual ~input_command() = default;
	virtual void execute()   = 0;
};

struct mouse_input_command : input_command
{
	sdl::Vec2i motion;
};

struct keyboard_input_command : input_command
{
	uint16_t modifier;
};

struct gamepad_button_command : input_command //this one exist for API consistency.
{
};

struct gamepad_axis_command : input_command
{
	int16_t value;
};