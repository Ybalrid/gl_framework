#pragma once

#include <glm/glm.hpp>

struct light
{
	glm::vec3 position{0.f};
	glm::vec3 direction{ glm::normalize(glm::vec3(1.f)) };
	
	//color values:
	glm::vec3 ambient{1.f};
	glm::vec3 diffuse{1.f};
	glm::vec3 specular{1.f};

	//attenuation parameters
	float constant{1.f};
	float linear{0.09f};
	float quadratic{0.032f};
};

struct directional_light : light
{
};

struct point_light : light
{
};