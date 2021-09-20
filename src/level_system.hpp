#pragma once

#include <string>

#include "json.hpp"
#include "scene.hpp"
#include "gltf_loader.hpp"
#include "script_system.hpp"
#include "physics_system.hpp"

class level_system
{
public:
  bool load_level(script_system& script_engine,  gltf_loader& gltf, scene& s, const std::string& level_name);

private:

  std::vector<physics_system::physics_proxy> proxies;
};