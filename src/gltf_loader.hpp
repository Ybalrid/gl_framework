#pragma once
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "renderable.hpp"

void tinygltf_freeimage_setup(tinygltf::TinyGLTF& gltf);
void tinygltf_resource_system_setup(tinygltf::TinyGLTF& gltf);

class gltf_loader
{
	tinygltf::TinyGLTF gltf;
	std::string error;
	std::string warning;

	shader* dshader = nullptr;

	//TODO don't store that here.
	std::vector<texture> gltf_textures{};

	void steal_guts(gltf_loader& loader);

public:

	gltf_loader() = default;
	~gltf_loader() = default;

	gltf_loader(shader& default_shader);
	gltf_loader(const gltf_loader&) = delete;
	gltf_loader& operator=(const gltf_loader&) = delete;
	gltf_loader(gltf_loader&& other) noexcept;
	gltf_loader& operator=(gltf_loader&& other) noexcept;

	bool load_model(const std::string& virtual_path, tinygltf::Model& model);
	renderable load_mesh(const std::string& virtual_path, int index);
	renderable load_mesh(const std::string& virtual_path, const std::string& name);

	//this is a bit unnecessary for OpenGL as theses identifier are the same
	static GLenum mode(GLenum input);
	static std::vector<float> get_vertices(const tinygltf::Model& model, int vertex_accessor_index, int texture_accessor_index, int normal_accessor_index);
	static std::vector<unsigned int> get_indices(const tinygltf::Model& model, int indices_accessor);
	GLuint load_to_gl_texture(const tinygltf::Image& color_image, bool srgb = true) const;
	renderable build_renderable(const tinygltf::Mesh& mesh, const tinygltf::Model& model);
};
