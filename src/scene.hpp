#pragma once

#include "node.hpp"

///A scene
struct scene
{
  ///The root of the scene
  node_ptr scene_root { create_node() };

  ///Run a predicate on some part of the scene
  template <class T, class... Args>
  void run_on_subgraph(node* start, T callable, Args... args)
  {
    callable(start, args...);
    for(size_t i = 0U; i < start->get_child_count(); ++i)
    {
      if(auto* child = start->get_child(i); child != nullptr) { run_on_subgraph(child, callable, args...); }
    }
  }

  ///Run a predicate on the whole graph
  template <class T, class... Args>
  void run_on_whole_graph(T callable, Args... args)
  {
    run_on_subgraph(scene_root.get(), callable, args...);
  }

  ///Find the node with said ID
  node* find_node(size_t id);

  ///Find the node in the children of node with the given id
  node* find_node_in_children(node* node, size_t id) const;
};
