#pragma once

#include <memory>
#include <string>

class script_system
{
public:
	script_system();
	~script_system();

	script_system(script_system&&) noexcept;
	script_system& operator=(script_system&&) noexcept;

	void register_imgui_library();
	void update(float delta);
	void eval_string(const std::string& input);
private:
	//ChaiScript is HUGE.
	//We are using a pimpl to hopefully speed-up the build time by having only one translation unit including chaiscript.h!!!!
	struct impl;
	std::unique_ptr<impl, void(*)(impl*)> pimpl;
	bool imgui_registered = false;
};
