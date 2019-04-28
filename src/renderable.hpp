#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>

#include "shader.hpp"
#include "texture.hpp"
#include "material.h"
#include "shader_program_manager.hpp"
#include "texture_manager.hpp"

#include <array>


using bounding_box = std::array<glm::vec3, 8>;

class renderable
{
public:
	struct vertex_buffer_extrema
	{
		glm::vec3 min, max;
		vertex_buffer_extrema() : min{0}, max{0}{}
		vertex_buffer_extrema(const glm::vec3& minimal, const glm::vec3& maximal) : min{minimal}, max{maximal} {}
		vertex_buffer_extrema(const vertex_buffer_extrema&) = default;
	};

private:
	shader_handle shader_program	= shader_program_manager::invalid_shader;
	texture_handle diffuse_texture  = texture_manager::invalid_texture;
	texture_handle specular_texture = texture_manager::invalid_texture;

	GLuint VAO = 0, VBO = 0, EBO = 0;
	GLenum draw_mode = GL_TRIANGLES, element_type = GL_UNSIGNED_INT;
	GLuint element_count = 0;

	glm::mat4 mvp = glm::mat4(1.f), model = glm::mat4(1.f), view = glm::mat4(1.f);
	glm::mat3 normal = glm::mat3(1.f);
	
	vertex_buffer_extrema bounds{};

	static constexpr GLuint vertex_position_location = 0;
	static constexpr GLuint vertex_texture_location  = 1;
	static constexpr GLuint vertex_normal_location   = 2;

	void steal_guts(renderable& other);

public:
	material mat;

	struct configuration
	{
		bool position : 1;
		bool texture : 1;
		bool normal : 1;
	};

	renderable() = default;

	renderable(shader_handle program,
			   const std::vector<float>& vertex_buffer,
			   const std::vector<unsigned int>& index_buffer,
			   vertex_buffer_extrema min_max_vpos,
			   configuration vertex_config,
			   size_t vertex_buffer_stride,
			   size_t vertex_coord_offset  = 0,
			   size_t texture_coord_offset = 0,
			   size_t normal_coord_offset  = 0,
			   GLenum draw_operation	   = GL_TRIANGLES,
			   GLenum buffer_usage		   = GL_STATIC_DRAW);

	void set_diffuse_texture(texture_handle t);
	void set_specular_texture(texture_handle t);

	~renderable();
	renderable(const renderable&) = delete;
	renderable& operator=(const renderable&) = delete;
	renderable(renderable&& other) noexcept;
	renderable& operator=(renderable&& other) noexcept;

	void draw() const;
	void set_mvp_matrix(const glm::mat4& matrix);
	void set_model_matrix(const glm::mat4& matrix);
	void set_view_matrix(const glm::mat4& matrix);

	vertex_buffer_extrema get_bounds() const;

	/*                              
							    2---Y-----6
							   /.   |    /|
							  / .   |   / |
							 /  .   |  /  |
							1-------+-5   |
							|   .   +-|---|------------X
							|   3../..|...7
							|  .  /   |  /
							| .  /    | /
							|.  /     |/
							0--/------4
							  Z

	The bounds of the object in space is stored as 2 3D vectors : bounds.min and bounds.max
	These vectors contains the minimal and maximal XYZ values of the whole object vertices.

	We need to describe a cuboid that effectively is an AABB of the object in model space.
	To describe the cuboid itself, we need 8 points, arbitrary chosen as this : 

	0 = {min.x, min.y, max.z};
	1 = {min.x, max.y, max.z};
	2 = {min.x, max.y, min.z};
	3 = {min.x, min.y, min.z};
	4 = {max.x, min.y, max.z};
	5 = {max.x, max.y, max.z};
	6 = {max.x, max.y, min.z};
	7 = {max.x, min.y, min.z};

	The obb is the "oriented bounding box". It's actually the object bounding box multiplied by it's model (=world)
	matrix. So with it's transform has been applied.

	 */

	inline bounding_box calculate_model_aabb() const;
	bounding_box get_world_obb(const glm::mat4& world_transform) const;
};
