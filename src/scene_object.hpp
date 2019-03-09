#pragma once

#include "renderable.hpp"
#include "camera.hpp"
#include "transform.hpp"

class scene_object
{
	renderable& mesh;

public:

	scene_object(renderable& r) : mesh{ r }
	{

	}

	void draw(const camera& camera, const glm::mat4& model)
	{
		//Set this object's matrix
		mesh.set_model_matrix(model);
		//Set the model_view_projection matrix. These are specific to each camera/object couple
		mesh.set_mvp_matrix(camera.get_view_projection_matrix() * model);

		mesh.draw();
	}

};
