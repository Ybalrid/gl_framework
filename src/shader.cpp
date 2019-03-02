#include "shader.hpp"
#include "resource_system.hpp"

std::vector<shader*> shader::shader_list{};

shader::shader(const std::string& vertex_shader_virtual_path, const std::string& fragment_shader_virtual_path)
{
	//Load source files as C strings
	auto vertex_source_data = resource_system::get_file(vertex_shader_virtual_path);
	auto fragment_source_data = resource_system::get_file(fragment_shader_virtual_path);
	vertex_source_data.push_back('\0');
	fragment_source_data.push_back('\0');
	const char* vertex_shader_cstr = reinterpret_cast<const char*>(vertex_source_data.data());
	const char* fragment_shader_cstr = reinterpret_cast<const char*>(fragment_source_data.data());

	//Compile shaders
	const auto vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const auto fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);

	glShaderSource(vertex_shader, 1, static_cast<const GLchar* const*>(&vertex_shader_cstr), nullptr);
	glShaderSource(fragment_shader, 1, static_cast<const GLchar* const*>(&fragment_shader_cstr), nullptr);
	glCompileShader(vertex_shader);
	glCompileShader(fragment_shader);

	//check shader compilation
	GLint success = 0;
	GLchar info_log[512];

	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if (!success)
	{
		glGetShaderInfoLog(vertex_shader, sizeof info_log, nullptr, info_log);
		throw std::runtime_error("Couldn't compile vertex shader " + std::string(info_log));
	}
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if (!success)
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
	if (!success)
	{
		glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
		throw std::runtime_error("Couldn't link shader program " + std::string(info_log));
	}

	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	//Object invariant:
	uniform_indices[int(uniform::mvp)] = glGetUniformLocation(program, "mvp");
	uniform_indices[int(uniform::model)] = glGetUniformLocation(program, "model");
	uniform_indices[int(uniform::normal)] = glGetUniformLocation(program, "normal");

	//Frame invariant:
	uniform_indices[int(uniform::light_position_0)] = glGetUniformLocation(program, "light_position_0");
	uniform_indices[int(uniform::camera_position)] = glGetUniformLocation(program, "camera_position");
	uniform_indices[int(uniform::view)] = glGetUniformLocation(program, "view");
	uniform_indices[int(uniform::projection)] = glGetUniformLocation(program, "projection");
	uniform_indices[int(uniform::gamma)] = glGetUniformLocation(program, "gamma");
	uniform_indices[int(uniform::time)] = glGetUniformLocation(program, "time");

	shader_list.push_back(this);
}

shader::~shader()
{
	if (glIsProgram(program) == GL_TRUE)
		glDeleteProgram(program);

	shader_list.erase(std::find(shader_list.begin(), shader_list.end(), this));
}

void shader::use() const
{
	glUseProgram(program);
}

void shader::use_0()
{
	glUseProgram(0);
}

void shader::set_uniform(uniform type, const glm::mat4& matrix) const
{
	glUniformMatrix4fv(uniform_indices[int(type)], 1, GL_FALSE, glm::value_ptr(matrix));
}

void shader::set_uniform(uniform type, const glm::mat3& matrix) const
{
	glUniformMatrix3fv(uniform_indices[int(type)], 1, GL_FALSE, glm::value_ptr(matrix));
}

void shader::set_uniform(uniform type, const glm::vec3& v) const
{
	glUniform3f(uniform_indices[int(type)], v.x, v.y, v.z);
}

void shader::set_uniform(uniform type, float v) const
{
	glUniform1f(uniform_indices[int(type)], v);
}
