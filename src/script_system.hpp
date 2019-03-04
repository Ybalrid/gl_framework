#pragma once

#include <memory>
class script_system
{
	struct impl;

	std::unique_ptr<impl> pimpl = nullptr;

public:
	script_system();
	~script_system();

	void register_imgui_library();

	void update(float delta);
};