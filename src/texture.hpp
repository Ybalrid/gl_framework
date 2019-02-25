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

	void bind(GLenum target = GL_TEXTURE_2D) const
	{
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
