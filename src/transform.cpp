#include "transform.hpp"

glm::mat4 transform::get_model() const
{
	if (dirty)
	{
		const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.f), position);
		const glm::mat4 rotation_matrix = glm::mat4_cast(glm::normalize(orientation));
		const glm::mat4 scaling_matrix = glm::scale(glm::mat4(1.f), scale);
		model = translation_matrix * rotation_matrix * scaling_matrix;
		dirty = false;
	}
	return model;
}

glm::vec3 transform::get_position() const
{
	return position;
}

glm::vec3 transform::get_scale() const
{
	return scale;
}

glm::quat transform::get_orientation() const
{
	return orientation;
}

void transform::set_position(const glm::vec3& new_position)
{
	if (glm::all(glm::equal(new_position,position))) 
		return;
	position = new_position;
	dirty = true;
}

void transform::set_scale(const glm::vec3& new_scale)
{
	if (glm::all(glm::equal(new_scale, scale))) 
		return;
	dirty = true;
	scale = new_scale;
}

void transform::set_orientation(const glm::quat& new_orientation)
{
	const auto normalized = normalize(new_orientation);
	if (glm::all(glm::equal(normalized, orientation))) 
		return;
	
	dirty = true;
	orientation = normalized;
}
