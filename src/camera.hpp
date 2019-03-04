#pragma once

#include "transform.hpp"

class camera
{
public:
	//parameters that can be changed at will
	enum projection_mode { perspective, ortho, hud, eye_vr };
	transform xform;
	projection_mode projection_type{ perspective };
	void (*vr_eye_projection_callback)(glm::mat4& projection_output, float near_clip, float far_clip) = nullptr;
	float near_clip = 0.1f;
	float far_clip = 1000.f;
	float fov = 45.f;

	//matrix getters
	glm::mat4 get_view_matrix() const;
	glm::mat4 get_projection_matrix() const;
	glm::mat4 get_view_projection_matrix() const;


	//Call this with the viewport geometry
	void update_projection(int viewport_w, int viewport_h, 
		int viewport_x = 0, int viewport_y = 0);
private:
	//enclosed projection matrix
	glm::mat4 projection{1.f};

};
