#include "script_system.hpp"
#include <chaiscript/chaiscript.hpp>
#include "gui.hpp"
#include <type_traits>

//We are using a pimpl idiom here. This limit chaiscript to be compiled only in this compilation unit
struct script_system::impl
{
	//The chaiscript interpreter instance is created here
	chaiscript::ChaiScript chai;

	//Printing
	impl()
	{
		std::cout << "Initialized scripting system using ChaiScript "
				  << chaiscript::version_major << '.'
				  << chaiscript::version_minor << '.'
				  << chaiscript::version_patch << '\n';
	}

	~impl()
	{
		std::cout << "Deinitialized scripting system\n";
	}

	//Get ref
	chaiscript::ChaiScript& get()
	{
		return chai;
	}
};

#include "ImGui__ChaiScript.h"
#include "chaiscript_glm.hpp"

script_system::script_system() :
 pimpl(new script_system::impl(), [](script_system::impl* ptr) { delete ptr; })
{
	auto& chai = pimpl->get();

	//Load external module
	chai.add(get_glm_module());
	install_additional_api();
}

script_system::~script_system()						   = default;
script_system::script_system(script_system&&) noexcept = default;
script_system& script_system::operator=(script_system&&) noexcept = default;

void script_system::register_imgui_library(gui* ui)
{
	auto& chai = pimpl->get();

	//install ImGui to chaiscript, set the gui_ptr. We are going to use this to show the console on screen
	chai.add(ImGui_GetChaiScriptModule());
	gui_ptr			 = ui;
	imgui_registered = true;

	//Pipe the console I/O to this
	//Declare a chaiscript function that pipe text output to the imgui console. Override the `print()` function to use it
	chai.add(fun([=](const std::string& str) { gui_ptr->push_to_console(str); }), "handle_output");
	chai.eval("global print = fun(x) { handle_output (to_string(x)); }");
	//add a function to clear the console
	chai.add(fun([=] { gui_ptr->clear_console(); }), "clear");

	//TODO move this boolean to C++ land
	chai.eval("var chai_window_is_open = true");
}

void script_system::update(float delta)
{
	(void)delta;
	auto& chai = pimpl->get();

	//This is just to show that ChaiScript can use the immediate mode gui
	chai.eval(R"chai(
if(chai_window_is_open)
{
	ImGui_Begin("From Chai", chai_window_is_open);
	ImGui_Text("The content of this window is generated from\n the `script_system::update()` method");
	ImGui_Text("This just flexes the bindings of ChaiScript to ImGui.");
	ImGui_Text("This is controlled by the value of the `chai_window_is_open` boolean");
	ImGui_End();
}
)chai");

	//TODO run update on "scriptable entities" whatever they are
}

//This is our entry point to run chaiscript code from outside
//TODO manage chaiscript exceptions that way
void script_system::eval_string(const std::string& input)
{
	auto& chai = pimpl->get();
	chai.eval(input);
}

//include and bind the rest of this engine API to ChaiScript
#include "transform.hpp"
#include "application.hpp"
#include "scene.hpp"
#include "node.hpp"

void script_system::install_additional_api()
{
	auto& chai = pimpl->get();

	using namespace chaiscript;
	using namespace glm;
	using namespace std;

	//Object transform
	chai.add(user_type<::transform>(), "transform");
	chai.add(constructor<::transform()>(), "transform");
	chai.add(fun(&transform::translate), "translate");
	chai.add(fun(static_cast<void (transform::*)(float, const glm::vec3&)>(&transform::rotate)), "rotate");
	chai.add(fun(&transform::scale), "scale");
	chai.add(fun(&transform::set_position), "set_position");
	chai.add(fun(&transform::set_orientation), "set_orientation");
	chai.add(fun(&transform::set_scale), "set_scale");
	chai.add(fun(&transform::get_position), "get_position");
	chai.add(fun(&transform::get_orientation), "get_orientation");
	chai.add(fun(&transform::get_scale), "get_scale");
	// clang-format off
	chai.add(fun([](const ::transform& t) -> std::string 
			{
				 const auto& p = t.get_position();
				 const auto& s = t.get_scale();
				 const auto& o = t.get_orientation();
				 return "position    (" + to_string(p.x) + ", " + to_string(p.y) + "," + to_string(p.z) + ")\n"
					 + "orientation (" + to_string(o.w) + ", " + to_string(o.x) + ", " + to_string(o.y) + ", " + to_string(o.z) + ")\n"
					 + "scale       (" + to_string(s.x) + ", " + to_string(s.y) + ", " + to_string(s.z) + ")";
			 }),
			 "to_string");
	// clang-format on

	chai.add(user_type<scene>(), "scene");

	// clang-format off
	chai.add(fun([](scene* s) -> node* 
			{
				 return (s->scene_root.get());
			 }),
			 "scene_root");
	// clang-format on

	chai.add(user_type<node>(), "node");
	chai.add(fun(&node::get_id), "get_id");
	chai.add(fun(&node::get_child_count), "get_child_count");
	chai.add(fun(&node::get_child), "get_child");
	chai.add(fun(&node::get_parent), "get_parent");
	chai.add(fun([](node* n) { return (&n->local_xform); }), "local_xform");
	chai.add(fun(&application::get_main_scene), "get_main_scene");
	chai.add(fun(&scene::find_node), "find_node");
	// clang-format off
	chai.add(fun([](node* n) -> std::string 
			{
				 std::string type;
				 n->visit([&](auto&& content) 
				 {
					 if constexpr(std::is_same_v<std::decay_t<decltype(content)>, std::monostate>)
						 type = "empty";
					 else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, scene_object>)
						 type = "scene_object";
					 else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, camera>)
						 type = "camera";
					 else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, light>)
						 type = "light";
					 else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, point_light>)
						 type = "point_light";
					 else
						 type = "???????? :O";
				 });
				 return "node : " + std::to_string(n->get_id())
					 + (n->get_parent() ? "\nchild of :" + std::to_string(n->get_parent()->get_id()) : "") + "\n"
					 + "type : " + type;
			 }),
			 "to_string");

	chai.add(fun([&](scene* s) -> std::string 
			{
				 std::string str;
				 auto to_string_node = chai.eval<std::function<std::string(node*)>>("to_string");
				 s->run_on_whole_graph([&](node* n) 
				 {
					 str += to_string_node(n);
					 str += "\n\n";
				 });
				 return str;
			 }),
			 "to_string");
	// clang-format on

	//TODO light integration
	//TODO physicsfs exploration?
}
