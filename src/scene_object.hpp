#pragma once

#include "renderable.hpp"
#include "camera.hpp"
#include "transform.hpp"
#include "mesh.hpp"

class scene_object
{
	mesh mesh_ {};

public:
	scene_object(mesh m) : mesh_ { m } {}

	std::vector<bounding_box> get_obb(const glm::mat4 model)
	{
		std::vector<bounding_box> output(mesh_.get_submeshes().size());
		for(size_t i = 0; i < output.size(); ++i)
			output[i] = (renderable_manager::get_from_handle(mesh_.get_submeshes()[i]).get_world_obb(model));
		return output;
	}

	mesh const& get_mesh() const { return mesh_; }
};
