#pragma once

#include "node.hpp"

struct scene
{
	node_ptr scene_root { create_node() };

	template <class T, class... Args>
	void run_on_subgraph(node* start, T callable, Args... args)
	{
		callable(start, args...);
		for(size_t i = 0U; i < start->get_child_count(); ++i)
		{
			if(auto* child = start->get_child(i); child != nullptr)
			{
				run_on_subgraph(child, callable, args...);
			}
		}
	}

	template <class T, class... Args>
	void run_on_whole_graph(T callable, Args... args)
	{
		run_on_subgraph(scene_root.get(), callable, args...);
	}

	node* find_node(size_t id);
	node* find_node_in_children(node* node, size_t id) const;
};