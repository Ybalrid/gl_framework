#pragma once

#include "camera.hpp"
#include "node.hpp"
#include "input_command.hpp"

#include <array>
#include <memory>

#include <SDL_scancode.h>

class camera_controller;

///Implementation of a command that translate the camera when "WASD" are pressed
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

///Implemenation of a command that change the speed of camera translation depending on how Shift is pressed
class camera_controller_run_modifier : public keyboard_input_command
{
public:
	camera_controller_run_modifier(camera_controller* owner);
	void execute() override;

private:
	camera_controller* owner_;
};

///Implementation of a command that rotate the camera depending on mouse movement
class camera_controller_mouse_command : public mouse_input_command
{
public:
	camera_controller_mouse_command(camera_controller* owner);
	void execute() override;

private:
	camera_controller* owner_;
};

///Camera controller, depending on input states, move the camera for one frame of simulation
class camera_controller
{
	//Internal states
	bool left		   = false;
	bool right		   = false;
	bool up			   = false;
	bool down		   = false;
	bool running	   = false;
	bool fly		   = false;
	float scaled_yaw   = 0;
	float scaled_pitch = 0;
	float scaler	   = 0.001f;

	//These classes needs to be able to change the variables above
	friend class camera_controller_command;
	friend class camera_controller_mouse_command;
	friend class camera_controller_run_modifier;

	node* controlled_camera_node = nullptr;

	///Command object storage
	std::array<std::unique_ptr<keyboard_input_command>, 2 * size_t(camera_controller_command::movement_type::count)>
		command_objects;
	std::unique_ptr<mouse_input_command> mouse_command_object;
	std::unique_ptr<keyboard_input_command> running_state_command;

public:
	///Create the controller, give it a node containing a camera
	camera_controller(node* camera_node);
	///Command to be executed for a keypress
	keyboard_input_command* press(camera_controller_command::movement_type type) const;
	///Command to be executed for a key release
	keyboard_input_command* release(camera_controller_command::movement_type type) const;
	///Command to be executed when the run button is pressed
	keyboard_input_command* run() const;
	///Command to be executed when the mouse is moved
	mouse_input_command* mouse_motion() const;

	///Apply the computed movement to the camera node
	void apply_movement(float delta_frame_second);

	///Walking speed in meters per second
	float walk_speed = 3.0f; // m.s^-1
	///Running speed in meters per second
	float run_speed = 13.f;
};
