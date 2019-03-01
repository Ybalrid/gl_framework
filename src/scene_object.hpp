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
		//Set this object's matrix
		mesh.set_model_matrix(xform.get_model());
		//Set the model_view_projection matrix. These are specific to each camera/object couple
		mesh.set_mvp_matrix(camera.view_projection_matrix() * xform.get_model());

		//Set the matrices that comes only from the camera 
		mesh.set_camera_position(camera.xform.get_position());
		mesh.set_view_matrix(camera.view_matrix());

		mesh.draw();
	}

};
