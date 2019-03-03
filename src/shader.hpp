#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <string>
#include "light.hpp"


class shader
{

public:

	///All settable uniforms in shaders
	enum class uniform
	{
		mvp,
		model,
		normal,
		view,
		projection,
		camera_position,
		gamma,
		time,
		main_directional_light,
		point_light_0,
		point_light_1,
		point_light_2,
		point_light_3,

		// Add uniforms on top of this one, and do not forget to glGetUniformLocation 
		// at the end of the constructor of the shader class
		MAX_UNIFORM_LOCATION_COUNT
	};

	struct directional_light_uniform_locations
	{
		int direction{ -1 };
		int ambient{ -1 };
		int diffuse{ -1 };
		int specular{ -1 };
	};

	struct point_light_uniform_locations
	{
		int position{ -1 };
		int constant{ -1 };
		int linear{ -1 };
		int quadratic{ -1 };
		int ambient{ -1 };
		int diffuse{ -1 };
		int specular{ -1 };
	};

	static constexpr const size_t NB_POINT_LIGHT{ 4 };


	///Construct a shader object. Take the location in the resource package of the source code
	shader(const std::string& vertex_shader_virtual_path, const std::string& fragment_shader_virtual_path);
	~shader();

	///Set the given uniform for *all* currently existing shader objects
	template<typename uniform_parameter>
	static void set_frame_uniform(uniform type, uniform_parameter param)
	{
		for (auto a_shader : shader_list)
		{
			a_shader->use();
			a_shader->set_uniform(type, param);
		}
		use_0();
	}

	shader(const shader&) = delete;
	shader& operator=(const shader&) = delete;

	void use() const;
	static void use_0();

	//One overload for every shader uniform available
	void set_uniform(uniform type, const glm::mat4& matrix) const;
	void set_uniform(uniform type, const glm::mat3& matrix) const;
	void set_uniform(uniform type, const glm::vec3& v) const;
	void set_uniform(uniform type, float v) const;
	void set_uniform(uniform type, const directional_light& light) const;
	void set_uniform(uniform type, const point_light& light) const;

private:
	GLuint program = 0;
	GLint uniform_indices[int(uniform::MAX_UNIFORM_LOCATION_COUNT)]{};
	directional_light_uniform_locations main_directional_light_uniform_locations;
	point_light_uniform_locations point_light_list_uniform_locations[NB_POINT_LIGHT];

	static std::vector<shader*> shader_list;
};
