#pragma once

#include <cpp-sdl2/event.hpp>
#include <imgui.h>

#include "input_command.hpp"
#include "gamepad_manager.hpp"

#include <array>

///Handle controllers, keyboard and mouse
class input_handler
{

  gamepad_manager gamepads;
  ///Access to ImGui
  ImGuiIO* imgui { nullptr };

  ///get Keyboard pressed command
  input_command* keypress(SDL_Scancode code, uint16_t mod);
  ///Keyboard released command command
  input_command* keyrelease(SDL_Scancode code, uint16_t mod);
  ///Get a command for any kind of event on keyboard
  input_command* keyany(SDL_Scancode code, uint16_t mod);
  ///Get mouse movement command
  input_command* mouse_motion(sdl::Vec2i relative_motion, sdl::Vec2i absolute_position);
  ///Get mouse clicked command
  input_command* mouse_button_down(uint8_t button, sdl::Vec2i absolute_position);
  ///Get mouse released command
  input_command* mouse_button_up(uint8_t button, sdl::Vec2i absolute_position);

  input_command* gamepad_button_down(uint8_t controller, SDL_GameControllerButton button);
  input_command* gamepad_button_up(uint8_t controller, SDL_GameControllerButton button);
  input_command* gamepad_axis_motion(uint8_t controller, SDL_GameControllerAxis axis, float value, axis_range range);

  ///storage for keyboard commands
  std::array<keyboard_input_command*, SDL_Scancode::SDL_NUM_SCANCODES> keypress_commands, keyrelease_commands, keyany_commands;
  ///storage for mouse commands
  std::array<mouse_input_command*, 5> mouse_button_down_commands, mouse_button_up_commands;
  ///Mouse motion command
  mouse_input_command* mouse_motion_command;

  std::array<std::array<gamepad_button_command*, SDL_CONTROLLER_BUTTON_MAX>, 4> gamepad_button_down_commands, gamepad_button_up_commands;
  std::array<std::array<gamepad_axis_motion_command*, SDL_CONTROLLER_AXIS_MAX>, 4> gamepad_axis_motion_commands;

  public:
  ///Get the command to execute for an SDL event
  input_command* process_input_event(const sdl::Event& e);
  ///Initialize imgui
  void setup_imgui();
  ///Construct the input handler
  input_handler();

  ///Bind keypress event type to command
  void register_keypress(SDL_Scancode code, keyboard_input_command* command);
  ///Bind keyrelease event type to command
  void register_keyrelease(SDL_Scancode code, keyboard_input_command* command);
  ///Bind keyany event type to command
  void register_keyany(SDL_Scancode code, keyboard_input_command* command);
  ///Bind mouse moved event to command
  void register_mouse_motion_command(mouse_input_command* command);
  ///Bind mouse down event to command
  void register_mouse_button_down_command(uint8_t sdl_mouse_button_name, mouse_input_command* command);
  ///Bind mouse up event to command
  void register_mouse_button_up_command(uint8_t sdl_mouse_button_name, mouse_input_command* command);

  void register_gamepad_button_down_command(SDL_GameControllerButton button, uint8_t controller_id, gamepad_button_command* command);
  void register_gamepad_button_up_command(SDL_GameControllerButton button, uint8_t controller_id, gamepad_button_command* command);
  void register_gamepad_axis_motion_command(SDL_GameControllerAxis axis, uint8_t controller_id, gamepad_axis_motion_command* command);
};
