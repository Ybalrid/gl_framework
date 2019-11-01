#include "gizmo.hpp"
#include "imgui.h"

glm::mat4 gizmo::manipulate(const glm::mat4& view_matrix,
                            const glm::mat4& projection_matrix,
                            const glm::mat4& world_model,
                            ImGuizmo::MODE mode,
                            ImGuizmo::OPERATION operation,
                            bool relative_to_parent,
                            glm::mat4 parent)
{
  glm::mat4 copy_model = world_model;
  ImGuizmo::Manipulate(
      glm::value_ptr(view_matrix), glm::value_ptr(projection_matrix), operation, mode, glm::value_ptr(copy_model));

  if(relative_to_parent) copy_model = glm::inverse(parent) * copy_model;

  return copy_model;
}

void gizmo::manipulate(node* n, camera* cam, ImGuizmo::MODE mode, ImGuizmo::OPERATION operation)
{
  const auto model               = n->get_world_matrix();
  const auto parent              = n->get_parent();
  const auto has_parent          = parent != nullptr;
  const auto proj                = cam->get_projection_matrix();
  const auto view                = cam->get_view_matrix();
  const auto parent_world_matrix = has_parent ? parent->get_world_matrix() : glm::mat4(1.f);

  const auto new_local_matrix = manipulate(view, proj, model, mode, operation, has_parent, parent_world_matrix);
  n->local_xform.set_to(new_local_matrix);
}

bool gizmo::want_mouse() { return ImGuizmo::IsOver() || ImGuizmo::IsUsing(); }

void gizmo::enable(bool state) { ImGuizmo::Enable(state); }

void gizmo::begin_frame()
{
  ImGuizmo::BeginFrame();
  const auto display_size = ImGui::GetIO().DisplaySize;
  ImGuizmo::SetRect(0, 0, display_size.x, display_size.y);
}
