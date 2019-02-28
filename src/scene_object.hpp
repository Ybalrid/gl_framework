#pragma once

#include "renderable.hpp"
#include "camera.hpp"
#include "transform.hpp"

class scene_object
{
	renderable& mesh;

public:

	transform xform;

	scene_object(renderable& r) : mesh{ r }
	{

	}

	void draw(const camera& camera)
	{
		mesh.set_model_matrix(xform.get_model());
		mesh.set_view_matrix(camera.view_matrix());
		mesh.set_mvp_matrix(camera.view_porjection_matrix() * xform.get_model());
		mesh.draw();
	}

};
