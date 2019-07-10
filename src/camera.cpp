#include <GL/glew.h>
#include "camera.hpp"

int camera::last_w = -1;
int camera::last_h = -1;
int camera::last_x = -1;
int camera::last_y = -1;

glm::mat4 camera::get_view_matrix() const { return glm::inverse(world_model_matrix); }

glm::mat4 camera::get_projection_matrix() const { return projection; }

glm::mat4 camera::get_view_projection_matrix() const { return projection * get_view_matrix(); }

void camera::set_world_matrix(const glm::mat4& matrix) { world_model_matrix = matrix; }

void camera::set_view_matrix(const glm::mat4 matrix) { world_model_matrix = glm::inverse(matrix); }

void camera::update_projection(int viewport_w, int viewport_h, int viewport_x, int viewport_y)
{
	const auto ratio = float(viewport_w) / float(viewport_h);
	switch(projection_type)
	{
		case perspective:
			set_gl_viewport(viewport_x, viewport_y, viewport_w, viewport_h);
			projection = glm::perspective(glm::radians(fov), ratio, near_clip, far_clip);
			break;

		case ortho: {
			//Orthographic porjection that respoct the viewport geometry, with (0,0) in the center of the screen,
			//in the normal opengl coordinates
			set_gl_viewport(viewport_x, viewport_y, viewport_w, viewport_h);
			const float width  = ratio / 2.f;
			const float height = 1 / 2.f;
			projection		   = glm::ortho(-width, width, -height, height, near_clip, far_clip);
		}
		break;

		case hud: {
			set_gl_viewport(viewport_x, viewport_y, viewport_w, viewport_h);
			//render geometry in screen space directly, with pixel values
			//we intentionally flip back the Y axis here so (0, 0) is the top left corner of the screen
			projection = glm::ortho(float(viewport_x),
									float(viewport_x + viewport_w),
									-float(viewport_y + viewport_h),
									float(viewport_y),
									near_clip,
									far_clip);
		}
		break;

		case eye_vr: {
			//This will **not** call glViewport for you
			if(vr_eye_projection_callback) vr_eye_projection_callback(projection, near_clip, far_clip);
		}
		break;
	}
}

void camera::set_gl_viewport(int x, int y, int w, int h)
{
	if(last_w != w || last_h != h || last_x != x || last_y != y)
	{
		glViewport(x, y, w, h);
		last_w = w;
		last_h = h;
		last_x = x;
		last_y = y;
	}
}
