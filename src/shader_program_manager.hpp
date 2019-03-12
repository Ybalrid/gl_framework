#pragma once

#include "shader.hpp"
#include <vector>
#include <iostream>

using shader_handle = size_t;

//Object pool for shader programs
class shader_program_manager
{
	std::vector<shader> shaders;
	std::vector<size_t> unallocated_shaders;

	static shader_program_manager* me;

public:

	shader_program_manager();

	~shader_program_manager();

	static shader& get_from_handle(shader_handle h);

	static void get_rid_of(shader_handle h);

	template<typename ... ConstructorArgs>
	static shader_handle construct_shader(ConstructorArgs ... args)
	{
		//no "unallocated" shader in the shaders array
		if (me->unallocated_shaders.empty())
		{
			me->shaders.emplace_back(args...);
			return me->shaders.size() - 1;
		}

		const shader_handle handle = me->unallocated_shaders.back();
		me->unallocated_shaders.pop_back();
		get_from_handle(handle) = shader(args...);
		return handle;
	}

	///Set the given uniform for *all* currently existing shader objects
	template<typename uniform_parameter>
	static void set_frame_uniform(shader::uniform type, uniform_parameter param)
	{
		for (auto& a_shader : me->shaders)
		{
			if (!a_shader.valid()) continue;
			a_shader.use();
			a_shader.set_uniform(type, param);
		}
		shader::use_0();
	}
};
