#pragma once

#include <GL/glew.h>
#include "image.hpp"
#include <array>
class texture
{
	static std::array<GLuint, 16> texture_bound_state;
	static GLint last_bound_texture_index;
	GLuint name = 0;
	void gen();
	void steal_guts(texture& o);
	static std::vector<texture*> texture_list;

public:
	texture();
	explicit texture(GLuint id);
	~texture();

	texture(const texture&) = delete;
	texture& operator=(const texture&) = delete;
	texture(texture&& o) noexcept;
	texture& operator=(texture&& o) noexcept;

	void bind(int index = 0, GLenum target = GL_TEXTURE_2D) const;
	void load_from(const image& img, bool is_sRGB = true, GLenum target = GL_TEXTURE_2D) const;
	void generate_mipmaps(GLenum target = GL_TEXTURE_2D) const;
	void set_filtering_parameters(GLenum target = GL_TEXTURE_2D) const;
	static void bind_0(GLenum target = GL_TEXTURE_2D);

	static void new_frame();

	GLuint get_glid() const;
};
