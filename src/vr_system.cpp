#include "vr_system.hpp"

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
