#include "vr_system.hpp"

GLuint vr_system::get_eye_framebuffer(eye output)
{
	if(output == eye::left) return eye_fbo[0];
	if(output == eye::right) return eye_fbo[1];
}

sdl::Vec2i vr_system::get_eye_framebuffer_size(eye output)
{
	if(output == eye::left) return eye_render_target_sizes[0];
	if(output == eye::right) return eye_render_target_sizes[1];
}

void vr_system::set_anchor(node* node)
{
	assert(node);
	vr_tracking_anchor = node;
}

camera* vr_system::get_eye_camera(eye output)
{
	if(output == eye::left) return eye_camera[0];
	if(output == eye::right) return eye_camera[1];
}
