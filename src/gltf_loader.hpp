#pragma once
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#ifdef USING_JETLIVE
#define TINYGLTF_NO_INCLUDE_JSON
#include "../third_party/jet-live/libs/json/json.hpp"
#endif
#include <tiny_gltf.h>

#include "renderable.hpp"
#include "renderable_manager.hpp"
#include "mesh.hpp"

#include <vector>
#include <tuple>
#include <unordered_map>

void tinygltf_freeimage_setup(tinygltf::TinyGLTF& gltf);
void tinygltf_resource_system_setup(tinygltf::TinyGLTF& gltf);

class gltf_loader
{
	tinygltf::TinyGLTF gltf;
	std::string error;
	std::string warning;
	shader_handle dshader = shader_program_manager::invalid_shader;

	void steal_guts(gltf_loader& loader);
	bool moved_from = false;

	std::unordered_map<int, texture_handle> model_texture_cache;

public:
	gltf_loader() = default;
	~gltf_loader();

	gltf_loader(shader_handle default_shader);
	gltf_loader(const gltf_loader&) = delete;
	gltf_loader& operator=(const gltf_loader&) = delete;
	gltf_loader(gltf_loader&& other) noexcept;
	gltf_loader& operator=(gltf_loader&& other) noexcept;

	bool load_model(const std::string& virtual_path, tinygltf::Model& model);
	mesh load_mesh(const std::string& virtual_path, int index);
	mesh load_mesh(const std::string& virtual_path, const std::string& name);
	mesh load_meshes(const std::string& virtual_path)
	{
		//TODO add meshes abstraction that contains multiple renderables
		return load_mesh(virtual_path, 0);
	}

	//this is a bit unnecessary for OpenGL as theses identifier are the same
	static GLenum mode(GLenum input);
	static std::tuple<std::vector<float>, renderable::vertex_buffer_extrema>
	get_vertices(const tinygltf::Model& model, int vertex_accessor_index, int texture_accessor_index, int normal_accessor_index);
	static std::vector<unsigned int> get_indices(const tinygltf::Model& model, int indices_accessor);
	GLuint load_to_gl_texture(const tinygltf::Image& color_image, bool srgb = true) const;
	mesh build_mesh(const tinygltf::Mesh& gltf_mesh, const tinygltf::Model& model);
};
