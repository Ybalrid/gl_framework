#pragma once

#include "shader.hpp"
#include <vector>

using shader_handle = std::vector<shader>::size_type;

//Object pool for shader programs
class shader_program_manager
{
	std::vector<shader> shaders;
	std::vector<size_t> unallocated_shaders;
	static shader_program_manager* manager;

public:
	static constexpr const shader_handle invalid_shader{ std::numeric_limits<shader_handle>::max() };

	shader_program_manager();
	~shader_program_manager();

	shader_program_manager(const shader_program_manager&) = delete;
	shader_program_manager(shader_program_manager&&)	  = delete;
	shader_program_manager& operator=(const shader_program_manager&) = delete;
	shader_program_manager& operator=(shader_program_manager&&) = delete;

	static shader& get_from_handle(shader_handle h);
	static void get_rid_of(shader_handle h);

	template <typename... ConstructorArgs>
	static shader_handle create_shader(ConstructorArgs... args)
	{
		//no "unallocated" shader in the shaders array
		if(manager->unallocated_shaders.empty())
		{
			manager->shaders.emplace_back(args...);
			return manager->shaders.size() - 1;
		}

		const shader_handle handle = manager->unallocated_shaders.back();
		manager->unallocated_shaders.pop_back();
		get_from_handle(handle) = shader(args...);
		return handle;
	}

	///Set the given uniform for *all* currently existing shader objects
	template <typename uniform_parameter>
	static void set_frame_uniform(shader::uniform type, uniform_parameter param)
	{
		for(auto& a_shader : manager->shaders)
		{
			if(!a_shader.valid()) continue;
			a_shader.use();
			a_shader.set_uniform(type, param);
		}
		shader::use_0();
	}
};
