#pragma once

#include <cpp-sdl2/vec2.hpp>

///Interface of an input command
struct input_command
{
  virtual ~input_command() = default;
  virtual void execute()   = 0;
};

///Command that  the mouse position and the button bitmask
struct mouse_input_command : input_command
{
  sdl::Vec2i relative_motion { 0, 0 }, absolute_position { 0, 0 };
  uint8_t click = 0;
};

///Keyboard input command that contains the current modifier
struct keyboard_input_command : input_command
{
  uint16_t modifier = 0;
};

///Button input command
struct gamepad_button_command : input_command //this one exist for API consistency.
{ };

enum class axis_range
{
  zero_to_one, //Like an analog trigger
  minus_one_to_one //Like a hotas throttle 
};

///Axis input command, contains the value of the axis
struct gamepad_1d_axis_command : input_command
{
  float value = 0;
  axis_range range;
};

///2D stick input command
struct gamepad_2d_stick_command : input_command
{
  float x = 0;
  float y = 0;
};
