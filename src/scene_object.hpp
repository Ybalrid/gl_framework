#pragma once

#include "renderable.hpp"

class scene_object
{
	glm::mat4 model = glm::mat4(1.f);
	renderable& mesh;

public:
	scene_object(renderable& r) : mesh{ r }
	{

	}

	void draw(glm::mat4 vp_matrix)
	{
		mesh.set_mvp_matrix(vp_matrix*model);
		mesh.draw();
	}

	void set_model(glm::mat4 matrix)
	{
		model = matrix;
	}
};
