#pragma once

#include "renderable.hpp"
#include "camera.hpp"
#include "transform.hpp"
#include "mesh.hpp"

///Object that is part of the scene
class scene_object
{
  ///The mesh
  mesh mesh_ {};

  public:
  ///The scene object
  scene_object(mesh m) : mesh_ { m } {}

  ///Get the oriented bounding box of the scene
  std::vector<bounding_box> get_obb(const glm::mat4 model)
  {
    std::vector<bounding_box> output(mesh_.get_submeshes().size());
    for(size_t i = 0; i < output.size(); ++i)
      output[i] = (renderable_manager::get_from_handle(mesh_.get_submeshes()[i]).get_world_obb(model));
    return output;
  }

  ///Get the mesh of the scene object
  mesh const& get_mesh() const { return mesh_; }
};
