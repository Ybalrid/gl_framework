#pragma once

#include <glm/glm.hpp>

struct directional_light
{
	glm::vec3 direction{-0.33f, 0.33f, -0.33f};

	//color values:
	glm::vec3 ambient{1.f};
	glm::vec3 diffuse{1.f};
	glm::vec3 specular{1.f};
};

struct point_light
{
	glm::vec3 position;

	//attenuation parameters
	float constant{1.f};
	float linear{0.09f};
	float quadratic{0.032f};

	//color values:
	glm::vec3 ambient{1.f};
	glm::vec3 diffuse{1.f};
	glm::vec3 specular{1.f};
};