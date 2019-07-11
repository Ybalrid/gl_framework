#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include "light.hpp"

class shader
{
	static GLuint last_used_program;

public:
	static float gamma;

	///All settable uniforms in shaders
	enum class uniform {
		mvp,
		model,
		normal,
		view,
		projection,
		light_space_matrix,
		camera_position,
		gamma,
		time,
		material_diffuse,
		material_specular,
		material_diffuse_color,
		material_specular_color,
		material_shininess,
		material_normal,
		main_directional_light,
		point_light_0,
		point_light_1,
		point_light_2,
		point_light_3,
		debug_color,

		// Add uniforms on top of this one, and do not forget to glGetUniformLocation
		// at the end of the constructor of the shader class
		MAX_UNIFORM_LOCATION_COUNT
	};

	///Uniform for a directional light
	struct directional_light_uniform_locations
	{
		int direction { -1 };
		int ambient { -1 };
		int diffuse { -1 };
		int specular { -1 };
	};

	///Uniform for a point light
	struct point_light_uniform_locations
	{
		int position { -1 };
		int constant { -1 };
		int linear { -1 };
		int quadratic { -1 };
		int ambient { -1 };
		int diffuse { -1 };
		int specular { -1 };
	};

	///Maximum number of lights in the system
	static constexpr size_t NB_POINT_LIGHT { 4 }; //this needs to match the same variable in the fragment shader

	//TODO redo texture binding system
	static constexpr int material_diffuse_texture_slot	= 0;
	static constexpr int material_specular_texture_slot = 1;
	static constexpr int material_normal_texture_slot	= 2;

	///Construct a shader object. Take the location in the resource package of the source code
	shader(const std::string& vertex_shader_virtual_path, const std::string& fragment_shader_virtual_path);

	///Construct an invalid and unallocated shader object
	shader();
	~shader();

	bool valid() const;

	shader(const shader&) = delete;
	shader& operator=(const shader&) = delete;
	shader(shader&& s) noexcept;
	shader& operator=(shader&& s) noexcept;

	void use() const;
	static void use_0();

	//One overload for every shader uniform available
	void set_uniform(uniform type, const glm::mat4& matrix) const;
	void set_uniform(uniform type, const glm::mat3& matrix) const;
	void set_uniform(uniform type, const glm::vec3& v) const;
	void set_uniform(uniform type, const glm::vec4& v) const;
	void set_uniform(uniform type, float v) const;
	void set_uniform(uniform type, int i) const;
	void set_uniform(uniform type, const directional_light& light) const;
	void set_uniform(uniform type, const point_light& light) const;

private:
	void steal_guts(shader& s);

	GLuint program = 0;
	GLint uniform_indices[int(uniform::MAX_UNIFORM_LOCATION_COUNT)] {};
	directional_light_uniform_locations main_directional_light_uniform_locations;
	point_light_uniform_locations point_light_list_uniform_locations[NB_POINT_LIGHT];
};
