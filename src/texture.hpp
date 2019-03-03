#pragma once

#include <GL/glew.h>
#include "image.hpp"
#include <algorithm>

class texture
{
	GLuint name = 0;
	void gen()
	{
		glGenTextures(1, &name);
	}

	void steal_guts(texture& o)
	{
		for(size_t i = 0; i < texture_list.size(); ++i)
		{
			if (texture_list[i]->name == name)
			{
				texture_list.erase(texture_list.begin() + i);
				break;
			}
		}

		if(name)
		{
			glDeleteTextures(1, &name);
		}

		name = o.name;
		o.name = 0;
		for(size_t i = 0; i < texture_list.size(); ++i)
		{
			if (texture_list[i]->name == o.name)
			{
				texture_list[i] = this;
				break;
			}
		}
	}
	static std::vector<texture*> texture_list;

public:

	texture()
	{
		gen();
		texture_list.push_back(this);
	}

	explicit texture(GLuint id) : name(id)
	{
		const auto texture_iterator = std::find_if(texture_list.begin(), texture_list.end(), [=](texture* t_ptr)
		{
			return t_ptr->name == id;
		});

		if (texture_iterator != texture_list.end())
		{
			throw std::runtime_error("Texutre " + std::to_string(id) + " already exist!");
		}

		texture_list.push_back(this);
	}

	~texture()
	{
		if (name)
		{
			glDeleteTextures(1, &name);
			texture_list.erase(std::find(texture_list.begin(), texture_list.end(), this));
		}
	}

	texture(const texture&) = delete;
	texture& operator=(const texture&) = delete;
	texture(texture&& o) noexcept
	{
		steal_guts(o);
	}

	texture& operator=(texture&& o) noexcept
	{
		steal_guts(o);
		return *this;
	}

	void bind(int index = 0, GLenum target = GL_TEXTURE_2D) const
	{
		glActiveTexture(GL_TEXTURE0 + index);
		glBindTexture(target, name);
	}

	void load_from(const image& img, bool is_sRGB = true, GLenum target = GL_TEXTURE_2D)
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

	void generate_mipmaps(GLenum target = GL_TEXTURE_2D)
	{
		bind();
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	static void bind_0(GLenum target = GL_TEXTURE_2D)
	{
		glBindTexture(target, 0);
	}

};
