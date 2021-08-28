#pragma once

#include <string>

#include "json.hpp"
#include "scene.hpp"
#include "gltf_loader.hpp"


class level_system
{
public:
  bool load_level(gltf_loader& gltf, scene& s, const std::string& level_name) const;

private:
};