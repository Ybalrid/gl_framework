#pragma once

#include <steam/steam_api.h>
#include <steam/isteaminput.h>
#include <steam/isteamutils.h>

#include "texture_manager.hpp"
#include <vector>

#include "input_command.hpp"

class steamworks
{
public:
  steamworks();
  ~steamworks();

  void update_steam_input();

  void load_controller_glyphs();

private:

  bool steamapi_initialized = false;
  ISteamInput* steamInput = nullptr;
  ISteamUtils* steamUtils   = nullptr;

  std::vector<texture_handle> controller_glyphs;

  void action_event_callback(SteamInputActionEvent_t* input_action_event);

};
