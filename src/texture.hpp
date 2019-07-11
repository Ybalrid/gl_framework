#pragma once

#include <GL/glew.h>
#include "image.hpp"
#include <array>

///A texture in GPU memory
class texture
{
	///texture units
	static std::array<GLuint, 16> texture_bound_state;
	///Las bound texture
	static GLint last_bound_texture_index;
	///OpenGL texture ID
	GLuint name = 0;
	///Generate the texture in GPU
	void gen();
	///move operation
	void steal_guts(texture& o);
	///List of textures in the system
	static std::vector<texture*> texture_list;

public:
	///Construct an empty texture
	texture();
	///construct a texture around an existing opengl texture
	explicit texture(GLuint id);
	///Destroy the texture
	~texture();
	///No copy
	texture(const texture&) = delete;
	///No copy
	texture& operator=(const texture&) = delete;
	///Move
	texture(texture&& o) noexcept;
	///Move
	texture& operator=(texture&& o) noexcept;
	///Bind the texture to the OpenGL state at given index
	void bind(int index = 0, GLenum target = GL_TEXTURE_2D) const;
	///load a texture from an image
	void load_from(const image& img, bool is_sRGB = true, GLenum target = GL_TEXTURE_2D) const;
	///Generate mim maps for the texture
	void generate_mipmaps(GLenum target = GL_TEXTURE_2D) const;
	///Set the filtering parameters
	void set_filtering_parameters(GLenum target = GL_TEXTURE_2D) const;
	///Unbid
	static void bind_0(GLenum target = GL_TEXTURE_2D);
	///Call this at new frame (forget the binding states)
	static void new_frame();
	///Get the OpenGL ID
	GLuint get_glid() const;
};
