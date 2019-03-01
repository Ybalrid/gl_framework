#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>


struct transform
{
	///Get the model matrix. Model matrix is cached until transform get's dirty.
	glm::mat4 get_model() const;

	///Get the internal position
	glm::vec3 get_position() const;

	///Get the internal scale
	glm::vec3 get_scale() const;

	///Get the internal orientation. This quaternion has been normalized
	glm::quat get_orientation() const;

	///Set the position. Set dirty flag.
	void set_position(const glm::vec3& new_position);


	///Set the scale. Set dirty flag
	void set_scale(const glm::vec3& new_scale);

	///Set the orientation. We will normalize this quaternion. Set dirty flag.
	void set_orientation(const glm::quat& new_orientation);

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
	mutable glm::mat4 model{ 1.f };

	//The internally stored absolute position, scale and orientation
	glm::vec3 position{ VEC_ZERO }, scale{ UNIT_SCALE };
	glm::quat orientation{ IDENTITY_QUAT };
};
