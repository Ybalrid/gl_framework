#include "renderable.hpp"

GLint renderable::last_bound_vao = 0;

void renderable::steal_guts(renderable& other)
{
	shader_program	 = other.shader_program;
	diffuse_texture	 = other.diffuse_texture;
	specular_texture = other.specular_texture;
	normal_texture	 = other.normal_texture;
	VAO				 = other.VAO;
	VBO				 = other.VBO;
	EBO				 = other.EBO;
	element_count	 = other.element_count;
	draw_mode		 = other.draw_mode;
	element_type	 = other.element_type;
	mvp				 = other.mvp;
	model			 = other.model;
	view			 = other.view;
	normal			 = other.normal;
	mat				 = other.mat;
	bounds			 = other.bounds;

	other.VAO = other.VBO = other.EBO = 0;
}

renderable::renderable(shader_handle program,
					   const std::vector<float>& vertex_buffer,
					   const std::vector<unsigned>& index_buffer,
					   vertex_buffer_extrema min_max,
					   configuration vertex_config,
					   size_t vertex_buffer_stride,
					   size_t vertex_coord_offset,
					   size_t texture_coord_offset,
					   size_t normal_coord_offset,
					   size_t tangent_coord_offset,
					   GLenum draw_operation,
					   GLenum buffer_usage) :
 shader_program(program),
 draw_mode(draw_operation), bounds { min_max }
{
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, GLsizei(sizeof(float) * vertex_buffer.size()), vertex_buffer.data(), buffer_usage);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, GLsizei(sizeof(unsigned int) * index_buffer.size()), index_buffer.data(), buffer_usage);
	if(vertex_config.position)
	{
		glVertexAttribPointer(vertex_position_location,
							  3,
							  GL_FLOAT,
							  GL_FALSE,
							  GLsizei(vertex_buffer_stride * sizeof(float)),
							  reinterpret_cast<void*>(vertex_coord_offset));
		glEnableVertexAttribArray(vertex_position_location);
	}
	if(vertex_config.texture)
	{
		glVertexAttribPointer(vertex_texture_location,
							  2,
							  GL_FLOAT,
							  GL_FALSE,
							  GLsizei(vertex_buffer_stride * sizeof(float)),
							  reinterpret_cast<void*>(texture_coord_offset * sizeof(float)));
		glEnableVertexAttribArray(vertex_texture_location);
	}
	if(vertex_config.normal)
	{
		glVertexAttribPointer(vertex_normal_location,
							  3,
							  GL_FLOAT,
							  GL_FALSE,
							  GLsizei(vertex_buffer_stride * sizeof(float)),
							  reinterpret_cast<void*>(normal_coord_offset * sizeof(float)));
		glEnableVertexAttribArray(vertex_normal_location);
	}
	if(vertex_config.tangent)
	{
		glVertexAttribPointer(vertex_tangent_location,
							  3,
							  GL_FLOAT,
							  GL_FALSE,
							  GLsizei(vertex_buffer_stride * sizeof(float)),
							  reinterpret_cast<void*>(tangent_coord_offset * sizeof(float)));
		glEnableVertexAttribArray(vertex_tangent_location);
	}
	glBindVertexArray(0);
	element_count = GLuint(index_buffer.size());
}

void renderable::set_diffuse_texture(texture_handle t) { diffuse_texture = t; }

void renderable::set_specular_texture(texture_handle t) { specular_texture = t; }

void renderable::set_normal_texture(texture_handle t) { normal_texture = t; }

renderable::~renderable()
{
	if(VBO) glDeleteBuffers(1, &VBO);
	if(EBO) glDeleteBuffers(1, &EBO);
	if(VAO) glDeleteVertexArrays(1, &VAO);
}

renderable::renderable(renderable&& other) noexcept { steal_guts(other); }

renderable& renderable::operator=(renderable&& other) noexcept
{
	steal_guts(other);
	return *this;
}

void renderable::draw() const
{
	//We need to have a shader and a texture!
	assert(shader_program != shader_program_manager::invalid_shader);

	auto& shader_object = shader_program_manager::get_from_handle(shader_program);

	//Setup our shader
	shader_object.use();
	shader_object.set_uniform(shader::uniform::mvp, mvp);		//projection * view * model
	shader_object.set_uniform(shader::uniform::model, model);	//world space model matrix
	shader_object.set_uniform(shader::uniform::normal, normal); //3x3 matrix extracted from(transpose(inverse(model)))
	shader_object.set_uniform(shader::uniform::material_shininess, mat.shininess);
	shader_object.set_uniform(shader::uniform::material_diffuse_color, mat.diffuse_color);
	shader_object.set_uniform(shader::uniform::material_specular_color, mat.specular_color);

	//bind material textures, fallback to the black texture if map doesn't exist. Do not forget any material slot here!
	if(diffuse_texture != texture_manager::invalid_texture)
		texture_manager::get_from_handle(diffuse_texture).bind(shader::material_diffuse_texture_slot);
	else
		texture_manager::get_from_handle(texture_manager::get_dummy_texture()).bind(shader::material_diffuse_texture_slot);
	shader_object.set_uniform(shader::uniform::material_diffuse, shader::material_diffuse_texture_slot);

	if(specular_texture != texture_manager::invalid_texture)
		texture_manager::get_from_handle(specular_texture).bind(shader::material_specular_texture_slot);
	else
		texture_manager::get_from_handle(texture_manager::get_dummy_texture()).bind(shader::material_specular_texture_slot);
	shader_object.set_uniform(shader::uniform::material_specular, shader::material_specular_texture_slot);

	if(normal_texture != texture_manager::invalid_texture)
		texture_manager::get_from_handle(normal_texture).bind(shader::material_normal_texture_slot);
	else
		texture_manager::get_from_handle(texture_manager::get_dummy_texture()).bind(shader::material_normal_texture_slot);
	shader_object.set_uniform(shader::uniform::material_normal, shader::material_normal_texture_slot);

	//bind object buffers and issue draw call
	if(last_bound_vao != VAO)
	{
		glBindVertexArray(VAO);
		last_bound_vao = VAO;
	}
	glDrawElements(draw_mode, element_count, element_type, nullptr);
}

void renderable::set_mvp_matrix(const glm::mat4& matrix) { mvp = matrix; }

void renderable::set_model_matrix(const glm::mat4& matrix)
{
	model  = matrix;
	normal = glm::mat3(glm::transpose(glm::inverse(matrix)));
}

void renderable::set_view_matrix(const glm::mat4& matrix) { view = matrix; }

renderable::vertex_buffer_extrema renderable::get_bounds() const { return bounds; }

inline std::array<glm::vec3, 8> renderable::calculate_model_aabb() const
{
	return { { { bounds.min.x, bounds.min.y, bounds.max.z },
			   { bounds.min.x, bounds.max.y, bounds.max.z },
			   { bounds.min.x, bounds.max.y, bounds.min.z },
			   { bounds.min.x, bounds.min.y, bounds.min.z },
			   { bounds.max.x, bounds.min.y, bounds.max.z },
			   { bounds.max.x, bounds.max.y, bounds.max.z },
			   { bounds.max.x, bounds.max.y, bounds.min.z },
			   { bounds.max.x, bounds.min.y, bounds.min.z } } };
}

std::array<glm::vec3, 8> renderable::get_world_obb(const glm::mat4& world_transform) const
{
	auto obb { calculate_model_aabb() };
	std::transform(obb.begin(), obb.end(), obb.begin(), [&](const glm::vec3& v) -> glm::vec3 {
		return world_transform * glm::vec4(v, 1.f);
	});

	return obb;
}
