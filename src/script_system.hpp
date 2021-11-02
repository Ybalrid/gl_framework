#pragma once

#include <memory>
#include <string>
#include <iostream>
#include "gui.hpp"
#include "node.hpp"

class script_node_behavior
{
  public:
  script_node_behavior()                            = default;
  script_node_behavior(const script_node_behavior&) = delete;
  script_node_behavior(script_node_behavior&&)      = delete;
  script_node_behavior& operator=(const script_node_behavior&) = delete;
  script_node_behavior& operator=(script_node_behavior&&) = delete;
  virtual ~script_node_behavior()                         = default;
  virtual void update()                                   = 0;
};

///Scripting engine
class script_system : public console_input_consumer
{
  public:
  script_system();
  ~script_system() override;

  script_system(script_system&&) noexcept;
  script_system& operator=(script_system&&) noexcept;

  ///Register the ImGui bindings
  void register_imgui_library(gui* ui);

  ///Run the update methods on the scripts
  void update(float delta);

  ///Evaluate a string inside the scripting engine
  void eval_string(const std::string& input) const;

  ///Evaluate a script file loaded from the resource system
  [[nodiscard]] bool evaluate_file(const std::string& path) const;

  ///Load a script from /scripts, and attach it to the node in question
  [[nodiscard]] bool attach_behavior_script(const std::string& name, node* attachment) const;

  ///functor that take a string and evalutate it
  bool operator()(const std::string& str) override
  {
    try
    {
      eval_string(str);
    }
    catch(const std::exception& e)
    {
      std::cerr << e.what() << "\n";
      gui_ptr->push_to_console(e.what());
      return false;
    }
    return true;
  }

  ///Get the name of every symbol available in the scripting engine on the global scope
  [[nodiscard]] std::vector<std::string> global_scope_object_names() const;

  private:
  //ChaiScript headers are HUGE.
  ///We are using a pimpl to hopefully speed-up the build time by having only one translation unit including chaiscript.h!!!!
  struct impl;
  std::unique_ptr<impl, void (*)(impl*)> pimpl;

  ///Is set to true if we have access to the GUI
  bool imgui_registered = false;

  ///pointer to the gui system
  gui* gui_ptr = nullptr;

  ///Run this once when setting up script engine bindings
  void install_additional_api();

  static size_t script_id;
};
