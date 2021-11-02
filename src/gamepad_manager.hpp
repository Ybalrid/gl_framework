#pragma once

#include <vector>
#include <cpp-sdl2/sdl.hpp>
#include "input_command.hpp"
#include <iostream>
#include <cstdio>

class gamepad_manager
{
  std::vector<sdl::GameController> gamepads;

  public:

  gamepad_manager()
  {
    detect_controllers();
    rumble(1, 1, 1000);
  }

  void detect_controllers()
  {
    gamepads = sdl::GameController::open_all_available_controllers();
  }

  void print_report()
  {
    for(const auto& controller  : gamepads)
    {
      std::cout << controller.name() << "\n";
      std::cout << "Left    X = " << controller.get_axis(SDL_CONTROLLER_AXIS_LEFTX) << "\n";
      std::cout << "Left    Y = " << controller.get_axis(SDL_CONTROLLER_AXIS_LEFTY) << "\n";
      std::cout << "Right   X = " << controller.get_axis(SDL_CONTROLLER_AXIS_RIGHTX) << "\n";
      std::cout << "Right   Y = " << controller.get_axis(SDL_CONTROLLER_AXIS_RIGHTY) << "\n";
      std::cout << "Trigger L = " << controller.get_axis(SDL_CONTROLLER_AXIS_TRIGGERLEFT) <<  "\n";
      std::cout << "Trigger R = " << controller.get_axis(SDL_CONTROLLER_AXIS_TRIGGERRIGHT) << "\n";

      for(int button = SDL_CONTROLLER_BUTTON_A; button < SDL_CONTROLLER_BUTTON_MAX; ++button)
        std::cout << '[' << (controller.get_button((SDL_GameControllerButton)button) ? 'X' : ' ') << ']';
      std::cout << "\n";
    }
  }

  void rumble(int id, float x, float d)
  {
    if(id >= gamepads.size())
        return;

    const uint16_t value = static_cast<uint16_t>(x * static_cast<float>(std::numeric_limits<uint16_t>::max()));

    gamepads[id].rumble(value, value, std::chrono::milliseconds(static_cast<int>(d) * 1000));
  }
};
