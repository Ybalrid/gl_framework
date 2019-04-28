#pragma once

#include "renderable.hpp"
#include "camera.hpp"
#include "transform.hpp"
#include "renderable_manager.hpp"

class scene_object
{
	renderable_handle mesh = renderable_manager::invalid_renderable;

public:
	scene_object(renderable_handle r) :
	 mesh { r }
	{
	}

	void draw(const camera& camera, const glm::mat4& model)
	{
		auto& mesh_object = renderable_manager::get_from_handle(mesh);
		//Set this object's matrix
		mesh_object.set_model_matrix(model);
		//Set the model_view_projection matrix. These are specific to each camera/object couple
		mesh_object.set_mvp_matrix(camera.get_view_projection_matrix() * model);

		mesh_object.draw();
	}

	bounding_box get_obb(const glm::mat4 model)
	{
		return renderable_manager::get_from_handle(mesh).get_world_obb(model);
	}
};
