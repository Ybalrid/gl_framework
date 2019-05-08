#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

struct light
{
	void set_position_from_world_mat(const glm::mat4& world_mat) { position = glm::vec3(world_mat[3]); }

	void set_direction_from_world_mat(const glm::mat4 world_mat)
	{
		const glm::vec3 neg_z(0, 0, -1.f);
		//Extract rotation part of the matrix
		const auto rotation = glm::normalize(glm::quat(world_mat));
		//reorient a vector "going though the screen"
		direction = rotation * neg_z;
	}

	glm::vec3 position { 0.f };
	glm::vec3 direction { glm::normalize(glm::vec3(1.f)) };

	//color values:
	glm::vec3 ambient { 1.f };
	glm::vec3 diffuse { 1.f };
	glm::vec3 specular { 1.f };

	//attenuation parameters
	float constant { 1.f };
	float linear { 0.09f };
	float quadratic { 0.032f };
};

struct directional_light : light
{};

struct point_light : light
{};