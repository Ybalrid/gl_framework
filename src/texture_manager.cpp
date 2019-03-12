#include "texture_manager.hpp"
#include <iostream>

texture_manager* texture_manager::manager = nullptr;

texture_manager::texture_manager()
{
	if(!manager) 
		manager = this;
	else 
		throw std::runtime_error("Cannont have more than one texture manager");

	std::cout << "Initialized texture manager\n";
}

texture_manager::~texture_manager()
{
	manager = nullptr;
	std::cout << "Deinitialized texture manager\n";
}

texture& texture_manager::get_from_handle(texture_handle t)
{
	if (t == invalid_texture)
		throw std::runtime_error("Cannot get from invalid texture handle");

	return manager->textures.at(t);
}

void texture_manager::get_rid_of(texture_handle t)
{
	if (t == invalid_texture) return;
	manager->get_from_handle(t);
	manager->unallocated_textures.push_back(t);
}
