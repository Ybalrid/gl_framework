#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class camera
{
public:
	enum projection_type { perspective, ortho };
private:
	projection_type current_projection{ projection_type::perspective };
	glm::mat4 model, projection;
	GLfloat near_clip = 0.1f, far_clip = 1000.f, fov = 45.f;

public:
	void set_projection_type(projection_type type)
	{
		current_projection = type;
	}

	glm::mat4 view_matrix() const
	{
		return glm::inverse(model);
	}

	glm::mat4 projection_matrix() const
	{
		return projection;
	}

	glm::mat4 view_porjection_matrix() const
	{
		return projection * view_matrix();
	}

	//Call this with the viewport geometry
	void update_projection(int viewport_w, int viewport_h, int viewport_x = 0, int viewport_y = 0)
	{
		const GLfloat ratio = float(viewport_w)/ float(viewport_h);
		glViewport(viewport_x, viewport_y, viewport_w, viewport_h);
		switch (current_projection)
		{
		case perspective:
			projection = glm::perspective(glm::radians(fov), ratio, near_clip, far_clip);
			break;
		case ortho:
		{
			float width = ratio / 2.f;
			float height = 1 / 2.f;
			projection = glm::ortho(-width, width, -height, height,
				near_clip,
				far_clip);
		}
		break;
		}
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