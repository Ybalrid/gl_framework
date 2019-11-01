#pragma once

#define NULL 0
#include "ImGuizmo.h"
#include "glm/glm.hpp"
#include "glm/gtc/type_ptr.hpp"

#include "node.hpp"
#include "camera.hpp"

struct gizmo
{
  ///Gizmo manipulation
  static glm::mat4 manipulate(const glm::mat4& view_matrix,
                              const glm::mat4& projection_matrix,
                              const glm::mat4& world_model,
                              ImGuizmo::MODE mode,
                              ImGuizmo::OPERATION operation,
                              bool relative_to_parent = false,
                              glm::mat4 parent        = glm::mat4(1.f));

  static void manipulate(node* n, camera* cam, ImGuizmo::MODE mode, ImGuizmo::OPERATION operation);

  ///Return true if gizmo is using the mouse
  static bool want_mouse();

  ///Permit to deactivate the gizmo
  static void enable(bool state = true);

  ///Call that at the begining of the frame
  static void begin_frame();
};
