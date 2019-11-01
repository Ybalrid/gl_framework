#include "shader_program_manager.hpp"
#include <iostream>

shader_program_manager* shader_program_manager::manager = nullptr;

shader_program_manager::shader_program_manager()
{
  if(!manager)
    manager = this;
  else
    throw std::runtime_error("Can't only create one shader manager!");

  std::cout << "Initialized shader program manager\n";
}

shader_program_manager::~shader_program_manager()
{
  manager = nullptr;
  std::cout << "Deinitialized shader program manager\n";
}

shader& shader_program_manager::get_from_handle(shader_handle h)
{
  if(h == invalid_shader) throw std::runtime_error("Cannot get from invalid shader handle");

  return manager->shaders.at(h);
}

void shader_program_manager::get_rid_of(shader_handle h)
{
  if(h == invalid_shader) return; //Nothing to do here
  manager->get_from_handle(h) = shader();
  manager->unallocated_shaders.push_back(h);
}
