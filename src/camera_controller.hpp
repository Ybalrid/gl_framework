#pragma once

#include "camera.hpp"
#include "node.hpp"
#include "input_command.hpp"

#include <array>
#include <memory>

#include <SDL_scancode.h>

class camera_controller;

class camera_controller_command : public keyboard_input_command
{
public:
	enum class movement_type { left, right, up, down, count };
	enum class action_type { pressed, released };

	camera_controller_command(camera_controller* owner, movement_type mvmt, action_type act);

	void execute() override;

private:
	const movement_type movement_type_;
	const action_type action_type_;
	camera_controller* owner_;
};

class camera_controller_run_modifier : public keyboard_input_command
{
public:
	camera_controller_run_modifier(camera_controller* owner);
	void execute() override;

private:
	camera_controller* owner_;
};

class camera_controller_mouse_command : public mouse_input_command
{
public:
	camera_controller_mouse_command(camera_controller* owner);
	void execute() override;

private:
	camera_controller* owner_;
};

class camera_controller
{
	bool left		   = false;
	bool right		   = false;
	bool up			   = false;
	bool down		   = false;
	bool running	   = false;
	bool fly		   = false;
	float scaled_yaw   = 0;
	float scaled_pitch = 0;
	float scaler	   = 0.001f;

	node* controlled_camera_node = nullptr;
	friend class camera_controller_command;
	friend class camera_controller_mouse_command;
	friend class camera_controller_run_modifier;

	std::array<std::unique_ptr<keyboard_input_command>, 2 * size_t(camera_controller_command::movement_type::count)>
		command_objects;
	std::unique_ptr<mouse_input_command> mouse_command_object;
	std::unique_ptr<keyboard_input_command> running_state_command;

public:
	camera_controller(node* camera_node);
	keyboard_input_command* press(camera_controller_command::movement_type type) const;
	keyboard_input_command* release(camera_controller_command::movement_type type) const;
	keyboard_input_command* run() const;
	mouse_input_command* mouse_motion() const;

	void apply_movement(float delta_frame_second);

	float walk_speed = 3.0f; // m.s^-1
	float run_speed	 = 13.f;
};
