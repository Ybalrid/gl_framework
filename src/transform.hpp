#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


struct transform
{
	///Get the model matrix. Model matrix is cached until transform get's dirty.
	inline glm::mat4 get_model() const
	{
		if (!dirty) return model;

		dirty = false;

		glm::mat4 translation_matrix = glm::translate(glm::mat4(1.f), position);
		glm::mat4 rotation_matrix = glm::mat4_cast(glm::normalize(orientation));
		glm::mat4 scaling_matrix = glm::scale(glm::mat4(1.f), scale);

		model = translation_matrix * rotation_matrix * scaling_matrix;
		return model;
	}

	///Get the internal position
	inline glm::vec3 get_position() const
	{
		return position;
	}

	///Get the internal scale
	inline glm::vec3 get_scale() const
	{
		return scale;
	}

	///Get the internal orientation. This quaternion has been normalized
	inline glm::quat get_orientation() const
	{
		return orientation;
	}

	///Set the position. Set dirty flag.
	inline void set_position(const glm::vec3& new_position)
	{
		position = new_position;
		bool dirty = true;
	}


	///Set the scale. Set dirty flag
	inline void set_scale(const glm::vec3& new_scale)
	{
		scale = new_scale;
		bool dirty = true;
	}

	///Set the orientation. We will normalize this quaternion. Set dirty flag.
	inline void set_orientation(const glm::quat& new_orientation)
	{
		orientation = glm::normalize(new_orientation);
		bool dirty = true;
	}

	///Positive X axis.
	inline static const glm::vec3 X_AXIS{ 1.f,0.f,0.f };
	///Positive Y axis.
	inline static const glm::vec3 Y_AXIS{ 0.f,1.f,0.f };
	///Positive Z axis.
	inline static const glm::vec3 Z_AXIS{ 0.f,0.f,1.f };
	///Vector of magnitude zero.
	inline static const glm::vec3 VEC_ZERO{ 0.f };
	///Vector with a scale of 1 on each directions.
	inline static const glm::vec3 UNIT_SCALE{ 1.f };
	///Quaternion encoding a "zero" rotation. Built from an identity matrix.
	inline static const glm::quat IDENTITY_QUAT{ (glm::quat(glm::mat4(1.f))) };


private:
	//This is the cached model matrix and a flag that signal if the model matrix is dirty
	mutable bool dirty = true;
	mutable glm::mat4 model{1.f};

	//The internally stored absolute position, scale and orientation
	glm::vec3 position{ VEC_ZERO }, scale{UNIT_SCALE};
	glm::quat orientation{IDENTITY_QUAT};
};
