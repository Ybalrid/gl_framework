#include "texture_manager.hpp"
#include <iostream>
#include <array>

texture_manager* texture_manager::manager	  = nullptr;
texture_handle texture_manager::dummy_texture = texture_manager::invalid_texture;

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
	if(t == invalid_texture) throw std::runtime_error("Cannot get from invalid texture handle");

	return manager->textures.at(t);
}

void texture_manager::get_rid_of(texture_handle t)
{
	if(t == invalid_texture) return;
	manager->get_from_handle(t);
	manager->unallocated_textures.push_back(t);
}

void texture_manager::initialize_dummy_texture()
{
	std::array<uint8_t, 2 * 2 * 4> black_square { 0 };
	GLuint dummy_texture_opengl_id;
	glGenTextures(1, &dummy_texture_opengl_id);
	glBindTexture(GL_TEXTURE_2D, dummy_texture_opengl_id);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 2, 2, 0, GL_RGBA, GL_UNSIGNED_BYTE, &black_square);
	glGenerateMipmap(GL_TEXTURE_2D);
	dummy_texture = create_texture(dummy_texture_opengl_id);
}
