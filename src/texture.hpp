#pragma once

#include <GL/glew.h>
#include "image.hpp"

class texture
{
	GLuint name = 0;
	void gen()
	{
		glGenTextures(1, &name);
	}

	void steal_guts(texture& o)
	{
		name = o.name;
		o.name = 0;
	}

public:
	texture()
	{
		gen();
	}

	explicit texture(GLuint id) : name(id)
	{
	}

	~texture()
	{
		if (name)
			glDeleteTextures(1, &name);
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

	void bind(GLenum target = GL_TEXTURE_2D) const
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(target, name);
	}

	void load_from(const image& img, GLenum target = GL_TEXTURE_2D)
	{
		bind(target);
		glTexImage2D(target,
			0,
			img.get_gl_type(img.get_type()),
			img.get_width(),
			img.get_height(),
			0,
			img.get_gl_type(img.get_type()),
			GL_UNSIGNED_BYTE,
			img.get_binary());
	}

	void generate_mipmaps(GLenum target = GL_TEXTURE_2D)
	{
		bind();
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	static void unbind(GLenum target = GL_TEXTURE_2D)
	{
		glBindTexture(target, 0);
	}

};
