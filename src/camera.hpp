#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class camera
{
	glm::mat4 model, projection;
	GLfloat near_clip = 0.1f, far_clip = 100.f, fov = 45.f;

public:

	glm::mat4 view_matrix()
	{
		return glm::inverse(model);
	}

	glm::mat4 projection_matrix()
	{
		return projection;
	}

	glm::mat4 view_porjection_matrix()
	{
		return projection * view_matrix();
	}

	void update_projection(int viewport_w, int viewport_h, int viewport_x = 0, int viewport_y = 0)
	{
		const GLfloat ratio = float(viewport_w)/ float(viewport_h);
		glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
		projection = glm::perspective(glm::radians(fov), ratio, near_clip, far_clip);
	}

	void set_fov(GLfloat degree)
	{
		fov = degree;
	}

	void set_model(glm::mat4 matrix)
	{
		model = matrix;
	}
};