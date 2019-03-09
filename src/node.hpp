#pragma once

#include <variant>
#include <vector>
#include <algorithm>

#include "transform.hpp"
#include "scene_object.hpp"
#include "camera.hpp"
#include "light.hpp"

#include <iostream>

//fw declare deleter
class node;
void destroy_node(node* n);

//declare node unique ptr with custom deleter
using node_ptr = std::unique_ptr < node, decltype(&destroy_node)> ;

class node
{
public:
	transform local_xform;

	//Define what a node can contain here
	using node_payload = std::variant<std::monostate,
		//list of things a node can be:
		scene_object, 
		camera, 
		light>;

	using child_list = std::vector<node_ptr>;
	node() : ID{ counter++ }{}
	~node() = default;

private:
	child_list children;
	node* parent = nullptr;
	glm::mat4 world_space_model{ 1.f };
	node_payload content;

	const size_t ID;
	static size_t counter;

public:

	glm::mat4 get_world_matrix() const
	{
		return world_space_model;
	}

	void update_world_matrix()
	{
		if (parent)
		{
			world_space_model = parent->world_space_model * local_xform.get_model();
		}
		else
		{
			world_space_model = local_xform.get_model();
		}

		for (auto& child : children)
			child->update_world_matrix();
	}

	size_t get_id() const
	{
		return ID;
	}

	size_t get_child_count() const
	{
		return children.size();
	}

	node* get_child(size_t index)
	{
		return children.at(index).get();
	}

	node* get_parent() const
	{
		return parent;
	}

	//this will move the pointer INTO the child array
	node* push_child(node_ptr&& child)
	{
		children.emplace_back(std::move(child));
		node* new_child = children.back().get();
		new_child->parent = this;
		return new_child;
	}

	node_ptr&& remove_child(size_t index)
	{
		return std::move(children.at(index));
	}

	void clean_child_list()
	{
		children.erase(std::remove(std::begin(children),
			std::end(children),
			nullptr),
			std::end(children));
	}
	
	template <typename T>
	T* get_if_is() const
	{
		return std::get_if<T>(&content);
	}

	template <typename T>
	T* get_if_is()
	{
		return std::get_if<T>(&content);
	}

	template<typename T>
	bool is_a() const
	{
		return std::holds_alternative<T>(content);
	}

	template<typename T>
	void assign(T&& object)
	{
		content.emplace<T>(std::move(object));
	}

	template<typename T>
	void visit(T visitor)
	{
		std::visit(visitor, content);
	}
};

//the famous deleter that has to be given to all the node_ptr objects
static void destroy_node(node* n)
{
	std::cout << "Destroying scene node " << n->get_id() << '\n';
	delete n;
}

//node factory function
inline node_ptr create_node()
{
	auto n = node_ptr(new node, destroy_node);
	std::cout << "Creating scene node   " << n->get_id() << '\n';
	return n;
}
