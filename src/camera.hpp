#pragma once

#include <GL/glew.h>
#include "transform.hpp"

class camera
{
public:
	enum projection_type { perspective, ortho, hud, eye_vr };
private:
	projection_type current_projection{ projection_type::perspective };
	glm::mat4 projection{1.f};

public:
	void (*vr_eye_projection_callback)(glm::mat4& projection_output, float near_clip, float far_clip) = nullptr;
	transform xform;
	float near_clip = 0.1f, far_clip = 1000.f, fov = 45.f;

	void set_projection_type(projection_type type)
	{
		current_projection = type;
	}

	glm::mat4 view_matrix() const
	{
		return glm::inverse(xform.get_model());
	}

	glm::mat4 projection_matrix() const
	{
		return projection;
	}

	glm::mat4 view_projection_matrix() const
	{
		return projection * view_matrix();
	}


	//Call this with the viewport geometry
	void update_projection(int viewport_w, int viewport_h, int viewport_x = 0, int viewport_y = 0)
	{
		const GLfloat ratio = float(viewport_w)/ float(viewport_h);
		switch (current_projection)
		{
		case perspective:
			glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
			projection = glm::perspective(
				glm::radians(fov), 
				ratio, 
				near_clip, 
				far_clip);
			break;
		case ortho:
		{
			//Orthographic porjection that respoct the viewport geometry, with (0,0) in the center of the screen,
			//in the normal opengl coordinates
			glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
			float width = ratio / 2.f;
			float height = 1 / 2.f;
			projection = glm::ortho(
				-width, 
				width, 
				-height, 
				height,
				near_clip,
				far_clip);
		}
		break;
		case hud:
			glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
			//render geometry in screen space directly, with pixel values
			//we intentionally flip back the Y axis here so (0, 0) is the top left corner of the screen
			projection = glm::ortho(
				float(viewport_x),
				float(viewport_x + viewport_w),
				-float(viewport_y + viewport_h),
				float(viewport_y),
				near_clip,
				far_clip);
			break;
		case eye_vr:
			//This will **not** call glViewport for you
			if (vr_eye_projection_callback)
				vr_eye_projection_callback(projection, near_clip, far_clip);
			break;
		}
	}
};
