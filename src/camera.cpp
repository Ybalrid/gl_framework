#include <GL/glew.h>
#include "camera.hpp"

glm::mat4 camera::get_view_matrix() const
{
	return glm::inverse(xform.get_model());
}

glm::mat4 camera::get_projection_matrix() const
{
	return projection;
}

glm::mat4 camera::get_view_projection_matrix() const
{
	return projection * get_view_matrix();
}

void camera::update_projection(int viewport_w, int viewport_h, int viewport_x, int viewport_y)
{
	const auto ratio = float(viewport_w) / float(viewport_h);
	switch (projection_type)
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
	{
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
	}
	break;

	case eye_vr:
	{
		//This will **not** call glViewport for you
		if (vr_eye_projection_callback)
			vr_eye_projection_callback(projection, near_clip, far_clip);
	}
	break;
	}
}
