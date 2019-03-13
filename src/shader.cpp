#include "shader.hpp"
#include "resource_system.hpp"
#include <algorithm>

float shader::gamma = 2.2f;

shader::shader(const std::string& vertex_shader_virtual_path, const std::string& fragment_shader_virtual_path)
{
	//Load source files as C strings
	auto vertex_source_data   = resource_system::get_file(vertex_shader_virtual_path);
	auto fragment_source_data = resource_system::get_file(fragment_shader_virtual_path);
	vertex_source_data.push_back('\0');
	fragment_source_data.push_back('\0');
	const char* vertex_shader_cstr   = reinterpret_cast<const char*>(vertex_source_data.data());
	const char* fragment_shader_cstr = reinterpret_cast<const char*>(fragment_source_data.data());

	//Compile shaders
	const auto vertex_shader   = glCreateShader(GL_VERTEX_SHADER);
	const auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertex_shader, 1, static_cast<const GLchar* const*>(&vertex_shader_cstr), nullptr);
	glShaderSource(fragment_shader, 1, static_cast<const GLchar* const*>(&fragment_shader_cstr), nullptr);
	glCompileShader(vertex_shader);
	glCompileShader(fragment_shader);

	//check shader compilation
	GLint success = 0;
	GLchar info_log[512];

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(vertex_shader, sizeof info_log, nullptr, info_log);
		throw std::runtime_error("Couldn't compile vertex shader " + std::string(info_log));
	}
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(fragment_shader, sizeof info_log, nullptr, info_log);
		throw std::runtime_error("Couldn't compile fragment shader " + std::string(info_log));
	}

	//link shaders into shader program
	program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
		throw std::runtime_error("Couldn't link shader program " + std::string(info_log));
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	//Object invariant:
	uniform_indices[int(uniform::mvp)]	= glGetUniformLocation(program, "mvp");
	uniform_indices[int(uniform::model)]  = glGetUniformLocation(program, "model");
	uniform_indices[int(uniform::normal)] = glGetUniformLocation(program, "normal");

	uniform_indices[int(uniform::material_diffuse)]		   = glGetUniformLocation(program, "material.diffuse");
	uniform_indices[int(uniform::material_specular)]	   = glGetUniformLocation(program, "material.specular");
	uniform_indices[int(uniform::material_shininess)]	  = glGetUniformLocation(program, "material.shininess");
	uniform_indices[int(uniform::material_diffuse_color)]  = glGetUniformLocation(program, "material.diffuse_color");
	uniform_indices[int(uniform::material_specular_color)] = glGetUniformLocation(program, "material.diffuse_color");

	//Frame invariant:
	uniform_indices[int(uniform::camera_position)] = glGetUniformLocation(program, "camera_position");
	uniform_indices[int(uniform::view)]			   = glGetUniformLocation(program, "view");
	uniform_indices[int(uniform::projection)]	  = glGetUniformLocation(program, "projection");
	uniform_indices[int(uniform::gamma)]		   = glGetUniformLocation(program, "gamma");
	uniform_indices[int(uniform::time)]			   = glGetUniformLocation(program, "time");

	//one directional lights
	main_directional_light_uniform_locations.direction = glGetUniformLocation(program, "main_directional_light.direction");
	main_directional_light_uniform_locations.ambient   = glGetUniformLocation(program, "main_directional_light.ambient");
	main_directional_light_uniform_locations.diffuse   = glGetUniformLocation(program, "main_directional_light.diffuse");
	main_directional_light_uniform_locations.specular  = glGetUniformLocation(program, "main_directional_light.specular");

	//A number of point lights
	for(size_t i = 0; i < NB_POINT_LIGHT; ++i)
	{
		std::string point_light_name = "point_light_list[" + std::to_string(i) + "].";

		point_light_list_uniform_locations[i].position  = glGetUniformLocation(program, (point_light_name + "position").c_str());
		point_light_list_uniform_locations[i].constant  = glGetUniformLocation(program, (point_light_name + "constant").c_str());
		point_light_list_uniform_locations[i].linear	= glGetUniformLocation(program, (point_light_name + "linear").c_str());
		point_light_list_uniform_locations[i].quadratic = glGetUniformLocation(program, (point_light_name + "quadratic").c_str());
		point_light_list_uniform_locations[i].ambient   = glGetUniformLocation(program, (point_light_name + "ambient").c_str());
		point_light_list_uniform_locations[i].diffuse   = glGetUniformLocation(program, (point_light_name + "diffuse").c_str());
		point_light_list_uniform_locations[i].specular  = glGetUniformLocation(program, (point_light_name + "specular").c_str());
	}

	set_uniform(uniform::material_diffuse, material_diffuse_texture_slot);
	set_uniform(uniform::material_specular, material_specular_texture_slot);
}

shader::shader()
{
}

shader::~shader()
{
	if(glIsProgram(program) == GL_TRUE)
		glDeleteProgram(program);
}

bool shader::valid() const
{
	return program != 0;
}

shader::shader(shader&& s) noexcept
{
	steal_guts(s);
}

shader& shader::operator=(shader&& s) noexcept
{
	steal_guts(s);
	return *this;
}

void shader::use() const
{
	glUseProgram(program);
}

void shader::use_0()
{
	glUseProgram(0);
}

//Avoid to set uniform on invalid opengl shader programs
#define shader_valid_check \
	if(!program) return
void shader::set_uniform(uniform type, const glm::mat4& matrix) const
{
	shader_valid_check;
	glUniformMatrix4fv(uniform_indices[int(type)], 1, GL_FALSE, value_ptr(matrix));
}

void shader::set_uniform(uniform type, const glm::mat3& matrix) const
{
	shader_valid_check;
	glUniformMatrix3fv(uniform_indices[int(type)], 1, GL_FALSE, value_ptr(matrix));
}

void shader::set_uniform(uniform type, const glm::vec3& v) const
{
	shader_valid_check;
	glUniform3f(uniform_indices[int(type)], v.x, v.y, v.z);
}

void shader::set_uniform(uniform type, float v) const
{
	shader_valid_check;
	glUniform1f(uniform_indices[int(type)], v);
}

void shader::set_uniform(uniform type, int i) const
{
	shader_valid_check;
	glUniform1i(uniform_indices[int(type)], i);
}

void shader::set_uniform(uniform type, const directional_light& light) const
{
	shader_valid_check;
	if(type != uniform::main_directional_light) return;

	glUniform3f(main_directional_light_uniform_locations.direction, light.direction.x, light.direction.y, light.direction.z);
	glUniform3f(main_directional_light_uniform_locations.ambient, light.ambient.r, light.ambient.g, light.ambient.b);
	glUniform3f(main_directional_light_uniform_locations.diffuse, light.diffuse.r, light.diffuse.g, light.diffuse.b);
	glUniform3f(main_directional_light_uniform_locations.specular, light.specular.r, light.specular.g, light.specular.b);
}

void shader::set_uniform(uniform type, const point_light& light) const
{
	shader_valid_check;
	const int index = [t = type] {
		switch(t)
		{
			default: return -1;
			case uniform::point_light_0: return 0;
			case uniform::point_light_1: return 1;
			case uniform::point_light_2: return 2;
			case uniform::point_light_3: return 3;
		};
	}();

	if(index < 0) return;

	const auto& point_light_location = point_light_list_uniform_locations[index];
	glUniform3f(point_light_location.position, light.position.x, light.position.y, light.position.z);
	glUniform1f(point_light_location.constant, light.constant);
	glUniform1f(point_light_location.linear, light.linear);
	glUniform1f(point_light_location.quadratic, light.quadratic);
	glUniform3f(point_light_location.ambient, light.ambient.r, light.ambient.g, light.ambient.b);
	glUniform3f(point_light_location.diffuse, light.diffuse.r, light.diffuse.g, light.diffuse.b);
	glUniform3f(point_light_location.specular, light.specular.r, light.specular.g, light.specular.b);
}

void shader::steal_guts(shader& s)
{
	program = s.program;

	std::copy(std::begin(s.uniform_indices), std::end(s.uniform_indices), std::begin(uniform_indices));
	std::copy(std::begin(s.point_light_list_uniform_locations), std::end(s.point_light_list_uniform_locations), std::begin(point_light_list_uniform_locations));
	main_directional_light_uniform_locations = s.main_directional_light_uniform_locations;
	s.program								 = 0; //this will invalidate the "moved from" program
}
