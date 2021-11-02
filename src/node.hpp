#pragma once

#include <variant>
#include <vector>
#include <algorithm>
#include <string>

#include "transform.hpp"
#include "scene_object.hpp"
#include "camera.hpp"
#include "light.hpp"

#include <iostream>
#include <type_traits>
#include <memory>
#include "audio_system.hpp"


class script_node_behavior;

//fw declare deleter
class node;
static void destroy_node(node* n);


//declare node unique ptr with custom deleter
using node_ptr = std::unique_ptr<node, decltype(&destroy_node)>;

class node
{
  public:
  transform local_xform;

  //Define what a node can contain here
  using node_payload = std::variant<std::monostate,
                                    //list of things a node can be:
                                    scene_object,
                                    camera,
                                    light,
                                    point_light,
                                    directional_light,
                                    audio_source,
                                    listener_marker>;

  using child_list = std::vector<node_ptr>;
  node() : ID { counter++ } { }
  node(const std::string& new_node_name) : node() { name = new_node_name; }
  ~node() = default;


  private:
  std::string name;
  child_list children;
  node* parent = nullptr;
  glm::mat4 world_space_model { 1.f };
  node_payload content;
  std::shared_ptr<script_node_behavior> behavior_script = nullptr;

  const size_t ID;
  static size_t counter;

  public:
  glm::mat4 get_world_matrix() const;
  void update_world_matrix();
  size_t get_id() const;
  size_t get_child_count() const;
  node* get_child(size_t index);
  node* get_parent() const;
  //this will move the unique pointer INTO the child array
  node* push_child(node_ptr&& child);
  node_ptr&& remove_child(size_t index);
  void clean_child_list();

  void attach_behavior_script(script_node_behavior* script_ptr);
  script_node_behavior* get_script_interface() const { return behavior_script.get(); }

  std::string get_name() const;

  template <typename T>
  const T* get_if_is() const
  {
    return std::get_if<T>(&content);
  }

  template <typename T>
  T* get_if_is()
  {
    return std::get_if<T>(&content);
  }

  template <typename T>
  bool is_a() const
  {
    return std::holds_alternative<T>(content);
  }

  template <typename T>
  T* assign(T&& object)
  {
    content.emplace<T>(std::move(object));
    return get_if_is<T>();
  }

  template <typename T>
  void visit(T visitor)
  {
    std::visit(visitor, content);
  }
};

//node factory function
node_ptr create_node();
node_ptr create_node(const std::string& new_node_name);
