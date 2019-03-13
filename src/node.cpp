#include "node.hpp"

size_t node::counter = 0;

glm::mat4 node::get_world_matrix() const
{
	return world_space_model;
}

void node::update_world_matrix()
{
	if(parent)
	{
		world_space_model = parent->world_space_model * local_xform.get_model();
	}
	else
	{
		world_space_model = local_xform.get_model();
	}

	visit([&](auto&& l) {
		if constexpr(std::is_same_v<std::decay_t<decltype(l)>, light>)
		{
			auto li = static_cast<light&>(l);
			li.set_position_from_world_mat(world_space_model);
			li.set_direction_from_world_mat(world_space_model);
		}
	});

	for(auto& child : children)
		child->update_world_matrix();
}

size_t node::get_id() const
{
	return ID;
}

size_t node::get_child_count() const
{
	return children.size();
}

node* node::get_child(size_t index)
{
	return children.at(index).get();
}

node* node::get_parent() const
{
	return parent;
}

node* node::push_child(node_ptr&& child)
{
	children.emplace_back(std::move(child));
	node* new_child   = children.back().get();
	new_child->parent = this;
	return new_child;
}

node_ptr&& node::remove_child(size_t index)
{
	return std::move(children.at(index));
}

void node::clean_child_list()
{
	children.erase(std::remove(std::begin(children),
							   std::end(children),
							   nullptr),
				   std::end(children));
}

node_ptr create_node()
{
	auto n = node_ptr(new node, destroy_node);
	//std::cout << "Creating scene node   " << n->get_id() << '\n';
	return n;
}

void destroy_node(node* n)
{
	//std::cout << "Destroying scene node " << n->get_id() << '\n';
	delete n;
}
