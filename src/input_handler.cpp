#include "input_handler.hpp"
#include <iostream>

#include "nameof.hpp"

input_command* input_handler::keypress(SDL_Scancode code, uint16_t modifier)
{
  if(keypress_commands[code]) keypress_commands[code]->modifier = modifier;
  return keypress_commands[code];
}

input_command* input_handler::keyrelease(SDL_Scancode code, uint16_t modifier)
{
  if(keyrelease_commands[code]) keyrelease_commands[code]->modifier = modifier;
  return keyrelease_commands[code];
}

input_command* input_handler::keyany(SDL_Scancode code, uint16_t modifier)
{
  if(keyany_commands[code]) keyany_commands[code]->modifier = modifier;
  return keyany_commands[code];
}

input_command* input_handler::mouse_motion(sdl::Vec2i relative_motion, sdl::Vec2i absolute_position)
{
  if(mouse_motion_command)
  {
    mouse_motion_command->relative_motion   = relative_motion;
    mouse_motion_command->absolute_position = absolute_position;
    mouse_motion_command->click             = 0;
  }
  return mouse_motion_command;
}

input_command* input_handler::mouse_button_down(uint8_t button, sdl::Vec2i absolute_position)
{
  const auto command = mouse_button_down_commands[button - 1];
  if(command)
  {
    command->absolute_position = absolute_position;
    command->relative_motion   = { 0, 0 };
    return command;
  }
  return nullptr;
}

input_command* input_handler::mouse_button_up(uint8_t button, sdl::Vec2i absolute_position)
{
  const auto command = mouse_button_up_commands[button - 1];
  if(command)
  {
    command->absolute_position = absolute_position;
    command->relative_motion   = { 0, 0 };
    return command;
  }
  return nullptr;
}

input_command* input_handler::gamepad_button_down(uint8_t controller, SDL_GameControllerButton button)
{
  return gamepad_button_down_commands[controller][button];
}

input_command* input_handler::gamepad_button_up(uint8_t controller, SDL_GameControllerButton button)
{
  return gamepad_button_up_commands[controller][button];
}

input_command* input_handler::gamepad_axis_motion(uint8_t controller, SDL_GameControllerAxis axis, float value, axis_range range)
{
  if(const auto command = gamepad_axis_motion_commands[controller][axis]; command)
  {
    command->range = range;
    command->value = value;
    return command;
  }

  return nullptr;
}

input_command* input_handler::process_input_event(const sdl::Event& e)
{
  switch(e.type)
  {
    //Keyboard events:
    case SDL_KEYDOWN:
      if(e.key.repeat
         || imgui && ((imgui->WantCaptureKeyboard || imgui->WantTextInput) && e.key.keysym.scancode != SDL_SCANCODE_GRAVE))
        break;
      if(const auto command = keypress(e.key.keysym.scancode, e.key.keysym.mod); command) return command;
      return keyany(e.key.keysym.scancode, e.key.keysym.mod);
    case SDL_KEYUP:
      if(e.key.repeat || imgui && (imgui->WantCaptureKeyboard || imgui->WantTextInput)) break;
      if(const auto command = keyrelease(e.key.keysym.scancode, e.key.keysym.mod); command) return command;
      return keyany(e.key.keysym.scancode, e.key.keysym.mod);

    //Mouse:
    case SDL_MOUSEMOTION:
      if(imgui && imgui->WantCaptureMouse) break;
      return mouse_motion({ e.motion.xrel, e.motion.yrel }, { e.motion.x, e.motion.y });
    case SDL_MOUSEBUTTONDOWN:
      if(imgui && imgui->WantCaptureMouse) break;
      return mouse_button_down(e.button.button, { e.button.x, e.button.y });
    case SDL_MOUSEBUTTONUP:
      if(imgui && imgui->WantCaptureMouse) break;
      return mouse_button_up(e.button.button, { e.button.x, e.button.y });

    //TODO controllers
    case SDL_CONTROLLERAXISMOTION:
      switch((SDL_GameControllerAxis)e.caxis.axis)
      {
        case SDL_CONTROLLER_AXIS_LEFTX:
        case SDL_CONTROLLER_AXIS_LEFTY:
        case SDL_CONTROLLER_AXIS_RIGHTX:
        case SDL_CONTROLLER_AXIS_RIGHTY:
          return gamepad_axis_motion(e.caxis.which, (SDL_GameControllerAxis)e.caxis.axis, float(e.caxis.value) / std::numeric_limits<Sint16> ::max(), axis_range::minus_one_to_one);
        case SDL_CONTROLLER_AXIS_TRIGGERLEFT:
        case SDL_CONTROLLER_AXIS_TRIGGERRIGHT:
          return gamepad_axis_motion(e.caxis.which, (SDL_GameControllerAxis)e.caxis.axis, float(e.caxis.value) / std::numeric_limits<Sint16> ::max(), axis_range::zero_to_one);
        default:
          return nullptr;
      }

    case SDL_CONTROLLERBUTTONDOWN:
      return gamepad_button_down(e.cbutton.which, (SDL_GameControllerButton)e.cbutton.button);
    case SDL_CONTROLLERBUTTONUP: std::cout << "controller button up\n";
      return gamepad_button_up(e.cbutton.which, (SDL_GameControllerButton)e.cbutton.button);
    default: break;
  }

  return nullptr;
}

input_handler::input_handler() :
 keypress_commands { nullptr }, keyrelease_commands { nullptr }, keyany_commands { nullptr },
 mouse_button_down_commands { nullptr }, mouse_button_up_commands { nullptr }, mouse_motion_command { nullptr },
 gamepad_button_down_commands { nullptr }, gamepad_button_up_commands { nullptr }, gamepad_axis_motion_commands { nullptr }
{ }

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

void input_handler::register_mouse_motion_command(mouse_input_command* command) { mouse_motion_command = command; }

void input_handler::register_mouse_button_down_command(uint8_t sdl_mouse_button_name, mouse_input_command* command)
{
  assert(sdl_mouse_button_name > 1 && sdl_mouse_button_name < 5
         && "This value should be one of SDL_BUTTON_{LEFT, MIDDLE, RIGHT, X1, X2}");
  mouse_button_down_commands[sdl_mouse_button_name - 1] = command;
}
void input_handler::register_mouse_button_up_command(uint8_t sdl_mouse_button_name, mouse_input_command* command)
{
  assert(sdl_mouse_button_name > 1 && sdl_mouse_button_name < 5
         && "This value should be one of SDL_BUTTON_{LEFT, MIDDLE, RIGHT, X1, X2}");
  mouse_button_up_commands[sdl_mouse_button_name - 1] = command;
}

void input_handler::register_gamepad_button_down_command(SDL_GameControllerButton button,
                                                         uint8_t controller_id,
                                                         gamepad_button_command* command)
{
  assert(controller_id < 4);
  gamepad_button_down_commands[controller_id][button] = command;
}

void input_handler::register_gamepad_button_up_command(SDL_GameControllerButton button,
                                                       uint8_t controller_id,
                                                       gamepad_button_command* command)
{
  assert(controller_id < 4);
  gamepad_button_up_commands[controller_id][button] = command;
}

void input_handler::register_gamepad_axis_motion_command(SDL_GameControllerAxis axis,
                                                         uint8_t controller_id,
                                                         gamepad_axis_motion_command* command)
{
  assert(controller_id < 4);
  gamepad_axis_motion_commands[controller_id][axis] = command;
}

void input_handler::setup_imgui() { imgui = &ImGui::GetIO(); }
