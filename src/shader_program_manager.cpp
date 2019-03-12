#include "shader_program_manager.hpp"

shader_program_manager* shader_program_manager::me = nullptr;

shader_program_manager::shader_program_manager()
{
	if(!me)
		me = this;
	else
		throw std::runtime_error("Can't only create one shader manager!");

	std::cout << "Initialized shader program manager\n";
}

shader_program_manager::~shader_program_manager()
{
	if(me) me = nullptr;
	std::cout << "Deinitialized shader program manager\n";
}

shader& shader_program_manager::get_from_handle(shader_handle h)
{
	return me->shaders.at(h);
}

void shader_program_manager::get_rid_of(shader_handle h)
{
	me->get_from_handle(h) = shader();
	me->unallocated_shaders.push_back(h);
}
