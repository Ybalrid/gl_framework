#include "camera_controller.hpp"

camera_controller::camera_controller(node* camera_node) : controlled_camera_node{camera_node}
{
	command_objects[0] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::left, camera_controller_command::action_type::pressed);
	command_objects[1] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::right, camera_controller_command::action_type::pressed);
	command_objects[2] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::up, camera_controller_command::action_type::pressed);
	command_objects[3] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::down, camera_controller_command::action_type::pressed);
	command_objects[4] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::left, camera_controller_command::action_type::released);
	command_objects[5] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::right, camera_controller_command::action_type::released);
	command_objects[6] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::up, camera_controller_command::action_type::released);
	command_objects[7] = std::make_unique<camera_controller_command>(this, camera_controller_command::movement_type::down, camera_controller_command::action_type::released);
}

input_command* camera_controller::press(camera_controller_command::movement_type type) const
{
	return command_objects[size_t(type)].get();
}

input_command* camera_controller::release(camera_controller_command::movement_type type) const
{
	return command_objects[size_t(type) + size_t(camera_controller_command::movement_type::count)].get();
}

void camera_controller::apply_movement() const
{
	std::cerr << "current states : " << left << right << up << down << '\n';
}

void camera_controller_command::execute()
{
	const bool new_state = action_type_ == action_type::pressed;

	#ifdef _DEBUG
	std::cerr << __func__ << ' ' << new_state << '\n';
	#endif

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
