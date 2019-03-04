#include "script_system.hpp"
#include "chaiscript/chaiscript.hpp"

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

#include "ImGui__ChaiScript.h";

script_system::script_system() 
{
	pimpl = std::make_unique<impl>();


}

script_system::~script_system()
{
}

void script_system::register_imgui_library()
{
	auto& chai = pimpl->get();
	auto ImGui_Module = ImGui_GetChaiScriptModule();
	chai.add(ImGui_Module);

}

void script_system::update(float delta)
{
	(void)delta;
	auto& chai = pimpl->get();

	chai.eval("ImGui_Text(\"Hello from ChaiScript!\");");
}
