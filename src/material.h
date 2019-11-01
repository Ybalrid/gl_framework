#pragma once

#include <glm/glm.hpp>

struct material
{
  float shininess = 32;
  glm::vec3 diffuse_color { 1.f };
  glm::vec3 specular_color { 1.f };
};