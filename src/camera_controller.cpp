#include <cpp-sdl2/sdl.hpp>
#include "camera_controller.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/norm.hpp>


camera_controller::camera_controller(node* camera_node) :
 controlled_camera_node { camera_node }
{
	command_objects[0] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::left, camera_controller_command::action_type::pressed);
	command_objects[1] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::right, camera_controller_command::action_type::pressed);
	command_objects[2] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::up, camera_controller_command::action_type::pressed);
	command_objects[3] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::down, camera_controller_command::action_type::pressed);
	command_objects[4] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::left, camera_controller_command::action_type::released);
	command_objects[5] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::right, camera_controller_command::action_type::released);
	command_objects[6] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::up, camera_controller_command::action_type::released);
	command_objects[7] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::down, camera_controller_command::action_type::released);
	mouse_command_object = std::make_unique<camera_controller_mouse_command>(this);
	running_state_command = std::make_unique<camera_controller_run_modifier>(this);

}

keyboard_input_command* camera_controller::press(camera_controller_command::movement_type type) const
{
	return command_objects[size_t(type)].get();
}

keyboard_input_command* camera_controller::release(camera_controller_command::movement_type type) const
{
	return command_objects[size_t(type) + size_t(camera_controller_command::movement_type::count)].get();
}

keyboard_input_command* camera_controller::run() const
{
	return running_state_command.get();
}

mouse_input_command* camera_controller::mouse_motion() const
{
	return mouse_command_object.get();
}


void camera_controller::apply_movement(float delta_frame_second)
{
	glm::vec3 movement_vector { 0.f };
	if(down)
		movement_vector.z += 1;
	if(up)
		movement_vector.z -= 1;
	if(left)
		movement_vector.x -= 1;
	if(right)
		movement_vector.x += 1;

	//Apply "WASD" movement
	if(transform::VEC_ZERO != movement_vector)
	{
		movement_vector = controlled_camera_node->local_xform.get_orientation() * movement_vector;
		movement_vector.y = 0; //Doing this is actually the only difference between walking and flying.
		movement_vector = glm::normalize(movement_vector) * (delta_frame_second * (running? run_speed : walk_speed));
		controlled_camera_node->local_xform.translate(movement_vector);
	}

	//Apply "mouse look" movement
	controlled_camera_node->local_xform.set_orientation(glm::quat(glm::vec3(scaled_pitch, scaled_yaw, 0.f))); //this euler angle will never gimbal lock
}

void camera_controller_command::execute()
{
	const bool new_state = action_type_ == action_type::pressed;

	switch(movement_type_)
	{
		case movement_type::left:
			owner_->left = new_state;
			break;
		case movement_type::right:
			owner_->right = new_state;
			break;
		case movement_type::up:
			owner_->up = new_state;
			break;
		case movement_type::down:
			owner_->down = new_state;
			break;

		default:
			break;
	}

}

void camera_controller_mouse_command::execute()
{
	owner_->scaled_yaw -= owner_->scaler * motion.x;
	owner_->scaled_pitch -= owner_->scaler * motion.y;
	owner_->scaled_pitch = glm::clamp(owner_->scaled_pitch, -glm::half_pi<float>(), glm::half_pi<float>());
}

void camera_controller_run_modifier::execute()
{
	owner_->running = modifier & KMOD_LSHIFT;
}
