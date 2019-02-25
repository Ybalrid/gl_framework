#pragma once

#include <GL/glew.h>
#include <vector>
#include "resource_system.hpp"

class shader
{
	GLuint program = 0;
public:
	shader(const std::string& vertex_shader_virtual_path, const std::string& fragment_shader_virtual_path)
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
		
	}

	~shader()
	{
		if(glIsProgram(program) == GL_TRUE)
			glDeleteProgram(program);
	}

	shader(const shader&) = delete;
	shader& operator=(const shader&) = delete;

	void use() const
	{
		glUseProgram(program);
	}

	static void use_0()
	{
		glUseProgram(0);
	}

	//todo set uniforms
};