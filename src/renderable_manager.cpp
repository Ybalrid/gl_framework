#include "renderable_manager.hpp"
#include <iostream>

renderable_manager* renderable_manager::manager = nullptr;

renderable_manager::renderable_manager()
{
	if (!manager)
		manager = this;
	else
		throw("Can only have one renderable manager");

	std::cout << "Initialized Renderable manager\n";
}

renderable_manager::~renderable_manager()
{
	manager = nullptr;
	std::cout << "Deinitialized Renderable manager\n";
}

renderable& renderable_manager::get_from_handle(renderable_handle r)
{
	if (r == invalid_renderable)
		throw std::exception("Cannot get invalid renderable");

	return manager->renderables.at(r);
}

void renderable_manager::get_rid_of(renderable_handle r)
{
	manager->unallocated_renderables.push_back(r);
	get_from_handle(r) = renderable();
}
