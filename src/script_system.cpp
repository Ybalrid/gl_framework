#include "script_system.hpp"
#include <chaiscript/chaiscript.hpp>
#include "gui.hpp"

struct script_system::impl
{
	chaiscript::ChaiScript chai;
	impl()
	{
		std::cout << "Initialized scripting system using ChaiScript "
			<< chaiscript::version_major << '.'
			<< chaiscript::version_minor << '.'
			<< chaiscript::version_patch << '\n';
	}

	chaiscript::ChaiScript& get()
	{
		return chai;
	}
};

#include "ImGui__ChaiScript.h"

script_system::script_system() : pimpl(new script_system::impl(), [](script_system::impl* ptr){delete ptr;})
{
}

script_system::~script_system() = default;
script_system::script_system(script_system&&) noexcept = default;
script_system& script_system::operator=(script_system&&) noexcept = default;

void script_system::register_imgui_library(gui* ui)
{
	auto& chai = pimpl->get();
	auto ImGui_Module = ImGui_GetChaiScriptModule();
	chai.add(ImGui_Module);
	gui_ptr = ui;

	//pring to the console with this
	chai.add(fun([&](const std::string& str){gui_ptr->push_to_console(str);}), "print");

	//to print integral numbers
	chai.add(fun([&](int n){gui_ptr->push_to_console(std::to_string(n));}), "print");
	chai.add(fun([&](float n){gui_ptr->push_to_console(std::to_string(n));}), "print");
	chai.add(fun([&](double n){gui_ptr->push_to_console(std::to_string(n));}), "print");
	chai.add(fun([&](char n){gui_ptr->push_to_console(std::to_string(n));}), "print");

}

void script_system::update(float delta)
{
	(void)delta;
	auto& chai = pimpl->get();

	//chai.eval("ImGui_Text(\"Hello from ChaiScript!\");");
}

void script_system::eval_string(const std::string& input)
{
	auto& chai = pimpl->get();
	chai.eval(input);
}
