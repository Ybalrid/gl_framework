#include "transform.hpp"

glm::mat4 transform::get_model() const
{
	if(dirty)
	{
		const glm::mat4 translation_matrix = glm::translate(glm::mat4(1.f), current_position);
		const glm::mat4 rotation_matrix	= glm::mat4_cast(glm::normalize(current_orientation));
		const glm::mat4 scaling_matrix	 = glm::scale(glm::mat4(1.f), current_scale);
		model							   = translation_matrix * rotation_matrix * scaling_matrix;
		dirty							   = false;
	}
	return model;
}

glm::vec3 transform::get_position() const
{
	return current_position;
}

glm::vec3 transform::get_scale() const
{
	return current_scale;
}

glm::quat transform::get_orientation() const
{
	return current_orientation;
}

void transform::set_position(const glm::vec3& new_position)
{
	if(glm::all(glm::equal(new_position, current_position)))
		return;
	current_position = new_position;
	dirty			 = true;
}

void transform::set_scale(const glm::vec3& new_scale)
{
	if(glm::all(glm::equal(new_scale, current_scale)))
		return;
	dirty		  = true;
	current_scale = new_scale;
}

void transform::set_orientation(const glm::quat& new_orientation)
{
	const auto normalized = normalize(new_orientation);
	if(glm::all(glm::equal(normalized, current_orientation)))
		return;

	dirty				= true;
	current_orientation = normalized;
}

void transform::translate(const glm::vec3& v)
{
	set_position(current_position + v);
}

void transform::scale(const glm::vec3& v)
{
	set_scale({ current_scale.x * v.x, current_scale.y * v.y, current_scale.z * v.z });
}

void transform::rotate(const glm::quat& q)
{
	set_orientation(current_orientation * q);
}

void transform::rotate(float angle, const glm::vec3& axis)
{
	set_orientation(glm::rotate(current_orientation, glm::radians(angle), axis));
}

std::string transform::to_string() const
{

	return "position    vec3(" + std::to_string(current_position.x) + ", " + std::to_string(current_position.y) + "," + std::to_string(current_position.z) + ")\n"
		+ "orientation quat(" + std::to_string(current_orientation.w) + ", " + std::to_string(current_orientation.x) + ", " + std::to_string(current_orientation.y) + ", " + std::to_string(current_orientation.z) + ")\n"
		+ "scale       vec3(" + std::to_string(current_scale.x) + ", " + std::to_string(current_scale.y) + ", " + std::to_string(current_scale.z) + ")";
}
