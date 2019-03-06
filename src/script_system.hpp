#pragma once

#include <memory>
#include <string>
#include <iostream>
#include "gui.hpp"
class script_system : public console_input_consumer
{
public:
	script_system();
	~script_system() override;

	script_system(script_system&&) noexcept;
	script_system& operator=(script_system&&) noexcept;

	void register_imgui_library(gui* ui);
	void update(float delta);
	void eval_string(const std::string& input);

	bool operator()(const std::string& str) override
	{
		try
		{
			eval_string(str);
		}
		catch (const std::exception& e)
		{
			std::cerr << e.what();
			if(gui_ptr) gui_ptr->push_to_console(e.what());
			return false;
		}
		return true;
	}

private:
	//ChaiScript is HUGE.
	//We are using a pimpl to hopefully speed-up the build time by having only one translation unit including chaiscript.h!!!!
	struct impl;
	std::unique_ptr<impl, void(*)(impl*)> pimpl;
	bool imgui_registered = false;

	gui* gui_ptr = nullptr;
};
