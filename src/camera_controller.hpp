#pragma once

#include "camera.hpp"
#include "node.hpp"
#include "input_command.hpp"

#include <array>
#include <memory>

#include <SDL_scancode.h>

class camera_controller;

class camera_controller_command : public input_command
{
public:
	enum class movement_type {
		left,
		right,
		up,
		down,
		count
	};
	enum class action_type { pressed,
							 released };
	camera_controller_command(camera_controller* owner, movement_type mvmt, action_type act) :
	 owner_ { owner }, movement_type_ {mvmt}, action_type_ { act } {}

	void execute() override;

private:
	const movement_type movement_type_;
	const action_type action_type_;
	camera_controller* owner_;
};

class camera_controller
{
	bool left  = false;
	bool right = false;
	bool up	= false;
	bool down  = false;

	node* controlled_camera_node = nullptr;
	friend class camera_controller_command;

	std::array<std::unique_ptr<input_command>, 2 * size_t(camera_controller_command::movement_type::count)> command_objects;

public:
	camera_controller(node* camera_node);
	input_command* press(camera_controller_command::movement_type type) const;
	input_command* release(camera_controller_command::movement_type type) const;

	void apply_movement() const;
};
