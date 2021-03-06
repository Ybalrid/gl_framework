#include "script_system.hpp"
#include <chaiscript/chaiscript.hpp>
#include "gui.hpp"
#include <type_traits>

//We are using a pimpl idiom here. This limit chaiscript to be compiled only in this compilation unit
struct script_system::impl
{
  impl(const impl&) = delete;
  impl(impl&&)      = delete;
  impl& operator=(const impl&) = delete;
  impl& operator=(impl&&) = delete;

  //The chaiscript interpreter instance is created here
  chaiscript::ChaiScript chai;

  //Printing
  impl()
  {
    std::cout << "Initialized scripting system using ChaiScript " << chaiscript::version_major << '.' << chaiscript::version_minor
              << '.' << chaiscript::version_patch << '\n';
  }

  ~impl() { std::cout << "Deinitialized scripting system\n"; }

  //Get ref
  chaiscript::ChaiScript& get() { return chai; }
};

#include "ImGui__ChaiScript.h"
#include "chaiscript_glm.hpp"

script_system::script_system() : pimpl(new script_system::impl(), [](script_system::impl* ptr) { delete ptr; })
{
  auto& chai = pimpl->get();

  //Load external module
  chai.add(get_glm_module());
  install_additional_api();
}

script_system::~script_system()                        = default;
script_system::script_system(script_system&&) noexcept = default;
script_system& script_system::operator=(script_system&&) noexcept = default;

void script_system::register_imgui_library(gui* ui)
{
  auto& chai = pimpl->get();

  //install ImGui to chaiscript, set the gui_ptr. We are going to use this to show the console on screen
  chai.add(ImGui_GetChaiScriptModule());
  gui_ptr          = ui;
  imgui_registered = true;

  //Pipe the console I/O to this
  //Declare a chaiscript function that pipe text output to the imgui console. Then override the `print()` function to use it
  chai.add(fun([=](const std::string& str) { gui_ptr->push_to_console(str); }), "handle_output");
  (void)chai.eval("global print = fun(x) { handle_output (to_string(x)); }");
  //After declaring a global (print) function here, any printing made by chaiscript will be sent to handle_output.
  //Handle output will systematically take advantage of a "to_string" function. All built-in chaiscript types have one.
  //Anything that can be written in the form of `object.to_string()` or `to_string(object)` will automatically work with this system.

  //add a function to clear the console
  chai.add(fun([=] { gui_ptr->clear_console(); }), "clear");
}

void script_system::update(float delta)
{
  //TODO run update on "scriptable entities" whatever they are
  (void)delta;
  auto& chai = pimpl->get();
  (void)chai;
}

//This is our entry point to run chaiscript code from outside
//TODO manage chaiscript exceptions that way
void script_system::eval_string(const std::string& input) const
{
  auto& chai = pimpl->get();
  (void)chai.eval(input);
}

#include <algorithm>
std::vector<std::string> script_system::global_scope_object_names() const
{
  auto& chai = pimpl->get();

  const auto& state     = chai.get_state().engine_state;
  const auto& functions = state.m_function_objects;
  const auto& globals   = state.m_global_objects;
  const auto& locals    = chai.get_locals();

  const auto size = functions.size() + globals.size() + locals.size() + 2;

  std::vector<std::string> output;
  output.reserve(size);

  //Add the true and false token to the list:
  output.emplace_back("true");
  output.emplace_back("false");

  for(const auto& [function_name, proxy] : functions)
  {
    (void)proxy;
    output.emplace_back(function_name);
  }

  for(const auto& [global_name, proxy] : globals)
  {
    (void)proxy;
    output.emplace_back(global_name);
  }

  for(const auto& [local_name, proxy] : locals)
  {
    (void)proxy;
    output.emplace_back(local_name);
  }

  //Sorting here will make the searching faster later
  std::sort(output.begin(), output.end());

  return output;
}

//include and bind the rest of this engine API to ChaiScript
#include "transform.hpp"
#include "application.hpp"
#include "scene.hpp"
#include "node.hpp"
#include "audio_system.hpp"

void script_system::install_additional_api()
{
  auto& chai = pimpl->get();

  using namespace chaiscript;
  using namespace glm;
  using namespace std;

  //Standard library input
  chai.add(bootstrap::standard_library::vector_type<std::vector<std::string>>("vector_string"));

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
  chai.add(fun(&transform::to_string), "to_string");
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
	                 else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, audio_source>)
	                    type = "audio_source";
	                 else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, listener_marker>)
	                    type = "audio_listener_marker";
					 else
						 type = std::string("???????? :O TODO add missing node type in ") + __FILE__ + " " + std::to_string(__LINE__);
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
                 auto to_string_xform = chai.eval<std::function<std::string(::transform*)>>("to_string");
				 s->run_on_whole_graph([&](node* n) 
				 {
					 str += to_string_node(n);
                     str += "\n";
                     str += to_string_xform(&n->local_xform);
					 str += "\n\n";
				 });
				 return str;
			 }),
			 "to_string");
  // clang-format on

  chai.add(fun(&sdl::Mouse::set_relative), "set_mouse_relative");
  chai.add(fun(&application::set_clear_color), "set_clear_color");

  //TODO light integration
  //TODO physicsfs exploration?

  chai.add(fun([](const std::string& root) { return resource_system::list_files(root, true); }), "list_files");

  chai.add(fun([](const std::string& root, bool recursive) { return resource_system::list_files(root, recursive); }),
           "list_files");

  //add a "to_string" method to std::vector<std::string>
  chai.add(fun([](const std::vector<std::string>& vector_of_strings) -> std::string {
             std::string output;
             for(const auto string : vector_of_strings)
             {
               output.append(string);
               output.append("\n");
             }
             return output;
           }),
           "to_string");
}
