#include "renderable.hpp"
#include <algorithm>

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

std::vector<float> generate_tangents(const std::vector<float>& vertex_buffer,
									 size_t stride,
									 size_t vertex_coord_offset,
									 size_t texture_coord_offset,
									 size_t normal_coord_offset)
{
	const size_t elements_count			= vertex_buffer.size() / stride;
	const size_t new_stride				= stride + 3; //(position.xyz, texture.st, normal.xyz) + tangent.xyz
	const size_t new_vertex_buffer_size = (vertex_buffer.size() / stride) * new_stride;
	std::vector<float> new_vertex_buffer(new_vertex_buffer_size);

	for(size_t i = 0; i < elements_count; ++i)
	{
		//copy existing vertex data
		memcpy(&new_vertex_buffer[i * new_stride], &vertex_buffer[i * stride], stride * sizeof(float));
		glm::vec3 normal, tangent, tangent_0, tangent_1;

		//extract normal vector
		memcpy(glm::value_ptr(normal), &vertex_buffer[i * stride + normal_coord_offset], 3 * sizeof(float));

		//extrapolate a tangent vector
		tangent_0 = glm::cross(normal, { 0.f, 0.f, 1.f });
		tangent_1 = glm::cross(normal, { 0.f, 1.f, 0.f });
		if(glm::length(tangent_0) > glm::length(tangent_1))
			tangent = tangent_0;
		else
			tangent = tangent_1;

		//TODO check if tangent aligned with uvs?
		(void)texture_coord_offset;
		(void)vertex_coord_offset;

		//write back tangent to new vertex buffer
		memcpy(&new_vertex_buffer[i * new_stride + stride], glm::value_ptr(tangent), 3 * sizeof(float));
	}

	return new_vertex_buffer;
}

void renderable::upload_to_gpu(const std::vector<float>& vertex_buffer,
							   const std::vector<unsigned>& index_buffer,
							   renderable::configuration vertex_config,
							   size_t vertex_buffer_stride,
							   size_t vertex_coord_offset,
							   size_t texture_coord_offset,
							   size_t normal_coord_offset,
							   size_t tangent_coord_offset,
							   GLenum buffer_usage)
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
	//Tangents are required for normal mapping. If vertex buffer doesn't have them, we will compute tem
	if(!vertex_config.tangent)
	{
		//compute a vertex buffer that includes tangent info
		const auto new_vertex_buffer = generate_tangents(
			vertex_buffer, vertex_buffer_stride, vertex_coord_offset, texture_coord_offset, normal_coord_offset);

		//Adjust the meta data to handle the fact that there's 3 more floats per vertex, and that the last 3 are the tangent vector
		const auto new_vertex_buffer_stride = vertex_buffer_stride + 3;
		const auto tangent_coord_offset		= vertex_buffer_stride;
		const auto new_vertex_config		= [=] {
			   auto new_vertex_config	 = vertex_config;
			   new_vertex_config.tangent = true;
			   return new_vertex_config;
		}();

		//Send to GPU
		upload_to_gpu(new_vertex_buffer,
					  index_buffer,
					  new_vertex_config,
					  new_vertex_buffer_stride,
					  vertex_coord_offset,
					  texture_coord_offset,
					  normal_coord_offset,
					  tangent_coord_offset,
					  buffer_usage);
	}
	else
	{
		//Use as-is
		upload_to_gpu(vertex_buffer,
					  index_buffer,
					  vertex_config,
					  vertex_buffer_stride,
					  vertex_coord_offset,
					  texture_coord_offset,
					  normal_coord_offset,
					  tangent_coord_offset,
					  buffer_usage);
	}
}

void renderable::set_diffuse_texture(texture_handle t) { diffuse_texture = t; }

void renderable::set_specular_texture(texture_handle t) { specular_texture = t; }

void renderable::set_normal_texture(texture_handle t) { normal_texture = t; }

texture_handle renderable::get_diffuse_texture() const { return diffuse_texture; }

texture_handle renderable::get_specular_texture() const { return specular_texture; }

texture_handle renderable::get_normal_texture() const { return normal_texture; }

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
	//TODO PBR in material
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

	submit_draw_call();
}

void renderable::submit_draw_call() const
{
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

glm::mat4 const& renderable::get_model_matrix() const { return model; }

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
