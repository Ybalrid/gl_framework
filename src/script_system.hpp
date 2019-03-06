#pragma once

#include <memory>
#include <string>

class script_system
{
	//ChaiScript is HUGE.
	//We are using a pimpl to hopefully speed-up the build time by having only one translation unit including chaiscript.h!!!!
	struct impl;
	std::unique_ptr<impl> pimpl = nullptr;
	bool imgui_registered = false;
public:
	script_system();
	~script_system();
	void register_imgui_library();
	void update(float delta);
	void eval_string(const std::string& input);
};
