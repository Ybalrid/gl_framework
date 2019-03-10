#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "texture.hpp"
#include "material.h"

class renderable
{
	shader* shader_program = nullptr;
	texture* diffuse_texture = nullptr;
	texture* specular_texture = nullptr;

	GLuint VAO = 0, VBO = 0, EBO = 0;
	GLenum draw_mode = GL_TRIANGLES, element_type = GL_UNSIGNED_INT;
	GLuint element_count = 0;

	glm::mat4 mvp = glm::mat4(1.f), model = glm::mat4(1.f), view = glm::mat4(1.f);
	glm::mat3 normal = glm::mat3(1.f);

	static constexpr GLuint vertex_position_location = 0;
	static constexpr GLuint vertex_texture_location = 1;
	static constexpr GLuint vertex_normal_location = 2;

	void steal_guts(renderable& other);

public:
	material mat;

	struct configuration
	{
		bool position : 1;
		bool texture : 1;
		bool normal : 1;
	};

	renderable(shader* program,
	           const std::vector<float>& vertex_buffer,
	           const std::vector<unsigned int>& index_buffer,
	           configuration vertex_config,
	           size_t vertex_buffer_stride,
	           size_t vertex_coord_offset  = 0,
	           size_t texture_coord_offset = 0,
	           size_t normal_coord_offset  = 0,
	           GLenum draw_operation       = GL_TRIANGLES,
	           GLenum buffer_usage         = GL_STATIC_DRAW);

	void set_diffuse_texture(texture* t);
	void set_specular_texture(texture* t);

	~renderable();
	renderable(const renderable&) = delete;
	renderable& operator=(const renderable&) = delete;
	renderable(renderable&& other) noexcept;
	renderable& operator=(renderable&& other) noexcept;

	void draw() const;
	void set_mvp_matrix(const glm::mat4& matrix);
	void set_model_matrix(const glm::mat4& matrix);
	void set_view_matrix(const glm::mat4& matrix);
	void set_camera_position(const glm::vec3& v) const;
};
