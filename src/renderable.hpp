#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "texture.hpp"

class renderable
{
	shader* shader_program = nullptr;
	texture* main_texture = nullptr;

	GLuint VAO = 0, VBO = 0, EBO = 0;
	GLenum draw_mode = GL_TRIANGLES, element_type = GL_UNSIGNED_INT;
	GLuint element_count = 0;

	glm::mat4 mvp = glm::mat4(1.f), model = glm::mat4(1.f), view = glm::mat4(1.f);
	glm::mat3 normal = glm::mat3(1.f);

	static constexpr GLuint vertex_coord_layout = 0;
	static constexpr GLuint texture_coord_layout = 1;
	static constexpr GLuint normal_coord_layout = 2;

	void steal_guts(renderable& other)
	{
		shader_program = other.shader_program;
		main_texture = other.main_texture;
		VAO = other.VAO;
		VBO = other.VBO;
		EBO = other.EBO;
		element_count = other.element_count;
		draw_mode = other.draw_mode;
		element_type = other.element_type;
		mvp = other.mvp;
		other.VAO = other.VBO = other.EBO = 0;
	}

public:
	static float gamma;

	struct configuration
	{
		bool position : 1;
		bool texture : 1;
		bool normal : 1;
	};

	renderable(shader& program, 
		texture& tex, 
		const std::vector<float>& vertex_buffer, 
		const std::vector<unsigned int>& index_buffer, 
		configuration vertex_config, 
		size_t vertex_buffer_stride, 
		size_t vertex_coord_offset = 0, 
		size_t texture_coord_offset = 0, 
		size_t normal_coord_offset = 0, 
		GLenum draw_operation = GL_TRIANGLES, 
		GLenum buffer_usage = GL_STATIC_DRAW):
		shader_program(&program), main_texture(&tex), draw_mode(draw_operation)
	{
		glGenBuffers(1, &VBO);
		glGenBuffers(1, &EBO);
		glGenVertexArrays(1, &VAO);
		glBindVertexArray(VAO);
		glBindBuffer(GL_ARRAY_BUFFER, VBO);
		glBufferData(GL_ARRAY_BUFFER, GLsizei(sizeof(float) * vertex_buffer.size()), vertex_buffer.data(), buffer_usage);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizei(sizeof(unsigned int) * index_buffer.size()), index_buffer.data(), buffer_usage);
		if (vertex_config.position)
		{
			glVertexAttribPointer(vertex_coord_layout, 3, GL_FLOAT, GL_FALSE, GLsizei(vertex_buffer_stride * sizeof(float)), reinterpret_cast<void*>(vertex_coord_offset));
			glEnableVertexAttribArray(vertex_coord_layout);
		}
		if (vertex_config.texture)
		{
			glVertexAttribPointer(texture_coord_layout, 2, GL_FLOAT, GL_FALSE, GLsizei(vertex_buffer_stride * sizeof(float)), reinterpret_cast<void*>(texture_coord_offset * sizeof(float)));
			glEnableVertexAttribArray(texture_coord_layout);
		}
		if (vertex_config.normal)
		{
			glVertexAttribPointer(normal_coord_layout, 3, GL_FLOAT, GL_FALSE, GLsizei(vertex_buffer_stride * sizeof(float)), reinterpret_cast<void*>(normal_coord_offset * sizeof(float)));
			glEnableVertexAttribArray(normal_coord_layout);
		}
		glBindVertexArray(0);
		element_count = GLuint(index_buffer.size());
	}
	
	~renderable()
	{
		if (VBO) glDeleteBuffers(1, &VBO);
		if (EBO) glDeleteBuffers(1, &EBO);
		if (VAO) glDeleteVertexArrays(1, &VAO);
	}

	renderable(const renderable&) = delete;
	renderable& operator=(const renderable&) = delete;

	renderable(renderable&& other) noexcept
	{
		steal_guts(other);
	}

	renderable& operator=(renderable&& other) noexcept
	{
		steal_guts(other);
		return *this;
	}
	 
	void draw() const
	{
		//We need to have a shader and a texture!
		assert(shader_program);
		assert(main_texture);

		//Setup our shader
		shader_program->use();
		shader_program->set_uniform(shader::uniform::mvp, mvp);			//projection * view * model
		shader_program->set_uniform(shader::uniform::model, model);		//world space model matrix
		shader_program->set_uniform(shader::uniform::normal, normal);	//3x3 matrix extracted from(transpose(inverse(model)))

		//TODO multitextuing
		main_texture->bind();

		//TODO material system

		//bind object buffers and issue draw call
		glBindVertexArray(VAO);
		glDrawElements(draw_mode, element_count, element_type, nullptr);
	}

	void set_mvp_matrix(const glm::mat4& matrix)
	{
		mvp = matrix;
	}

	void set_model_matrix(const glm::mat4& matrix)
	{
		model = matrix;
		normal = glm::mat3(glm::transpose(glm::inverse(matrix)));
	}

	void set_view_matrix(const glm::mat4& matrix)
	{
		view = matrix;
	}

	void set_light_0_position(const glm::vec3& v) const
	{
		shader_program->use();
		shader_program->set_uniform(shader::uniform::light_position_0, v);
	}

	void set_camera_position(const glm::vec3& v) const
	{
		shader_program->use();
		shader_program->set_uniform(shader::uniform::camera_position, v);
	}
};