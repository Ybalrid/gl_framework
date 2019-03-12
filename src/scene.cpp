#include "scene.hpp"

node* scene::find_node(size_t id)
{
	return find_node_in_children(scene_root.get(), id);
}

node* scene::find_node_in_children(node* node, size_t id) const
{
	if(node->get_id() == id) return node;
	const auto child_count = node->get_child_count();
	for(size_t i = 0; i < child_count; ++i)
	{
		auto found = find_node_in_children(node->get_child(i), id);
		if(found) return found;
	}

	return nullptr;
}
