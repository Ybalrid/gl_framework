#pragma once

#include "renderable.hpp"
#include "camera.hpp"

class scene_object
{
	glm::mat4 model = glm::mat4(1.f);
	renderable& mesh;

public:
	scene_object(renderable& r) : mesh{ r }
	{

	}

	void draw(const camera& camera)
	{
		mesh.set_model_matrix(model);
		mesh.set_view_matrix(camera.view_matrix());
		mesh.set_mvp_matrix(camera.view_porjection_matrix() *model);
		mesh.draw();
	}

	void set_model(glm::mat4 matrix)
	{
		model = matrix;
	}

	glm::mat4 get_model() const
	{
		return model;
	}
};
