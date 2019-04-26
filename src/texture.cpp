#include "texture.hpp"
#include <algorithm>

std::vector<texture*> texture::texture_list {};
GLuint texture::last_bound_texture = 0;
GLint texture::last_bound_texture_index = 0;

void texture::gen()
{
	glGenTextures(1, &name);
}

void texture::steal_guts(texture& o)
{
	for(size_t i = 0; i < texture_list.size(); ++i)
	{
		if(texture_list[i]->name == name)
		{
			texture_list.erase(texture_list.begin() + long(i));
			break;
		}
	}

	if(name)
	{
		glDeleteTextures(1, &name);
	}

	name   = o.name;
	o.name = 0;
	for(size_t i = 0; i < texture_list.size(); ++i)
	{
		if(texture_list[i]->name == o.name)
		{
			texture_list[i] = this;
			break;
		}
	}
}

texture::texture()
{
	gen();
	texture_list.push_back(this);
}

texture::texture(GLuint id) :
 name(id)
{
	const auto texture_iterator = std::find_if(texture_list.begin(), texture_list.end(), [=](texture* t_ptr) {
		return t_ptr->name == id;
	});

	if(texture_iterator != texture_list.end())
	{
		throw std::runtime_error("Texutre " + std::to_string(id) + " already exist!");
	}

	texture_list.push_back(this);
}

texture::~texture()
{
	if(name)
	{
		glDeleteTextures(1, &name);
		texture_list.erase(std::find(texture_list.begin(), texture_list.end(), this));
	}
}

texture::texture(texture&& o) noexcept
{
	steal_guts(o);
}

texture& texture::operator=(texture&& o) noexcept
{
	steal_guts(o);
	return *this;
}

void texture::bind(int index, GLenum target) const
{
	//Avoid redundant opengl call
	if(last_bound_texture != name || last_bound_texture_index != index)
	{
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(target, name);
		last_bound_texture = name;
		last_bound_texture_index = index;
	}
}

void texture::load_from(const image& img, bool is_sRGB, GLenum target) const
{
	bind(0, target);
	glTexImage2D(target,
				 0,
				 is_sRGB ? GL_SRGB_ALPHA : GL_RGBA,
				 img.get_width(),
				 img.get_height(),
				 0,
				 GL_RGBA,
				 GL_UNSIGNED_BYTE,
				 img.get_binary());
}

void texture::generate_mipmaps(GLenum target) const
{
	bind(0, target);
	glGenerateMipmap(target);
}

void texture::set_filtering_parameters(GLenum target)
{
	bind(0, target);

	//Can we set an anisotropic filtering on texture?
	if(GLEW_EXT_texture_filter_anisotropic) //Just be prudent, this extension is only from 1999...
	{
		float anisotropy = 0;
		//querry driver for maximum anisotropy
		glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &anisotropy);

		//apply if value sucessfully acquired
		if(anisotropy != 0)
			glTexParameterf(target, GL_TEXTURE_MAX_ANISOTROPY_EXT, anisotropy);
	}

	glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}

void texture::bind_0(GLenum target)
{
	glBindTexture(target, 0);
	last_bound_texture = 0;
	last_bound_texture_index = 0;
}
