#pragma once
#define TINYGLTF_NO_STB_IMAGE
#define TINYGLTF_NO_STB_IMAGE_WRITE
#include <tiny_gltf.h>

#include "renderable.hpp"
#include "resource_system.hpp"

void tinygltf_freeimage_setup(tinygltf::TinyGLTF& gltf);
void tinygltf_resource_system_setup(tinygltf::TinyGLTF& gltf);


class gltf_loader
{
	tinygltf::TinyGLTF gltf;
	std::string error;
	std::string warning;

	shader& dshader;
	texture& dtexture;

public:
	gltf_loader(shader& default_shader, texture& default_texture) : dshader{ default_shader }, dtexture{ default_texture }
	{
		//This register our custom ImageLoader with tinygltf
		tinygltf_freeimage_setup(gltf);
		tinygltf_resource_system_setup(gltf);
	}

	bool load_model(const std::string& virtual_path, tinygltf::Model& model)
	{
		const auto ext = virtual_path.substr(virtual_path.find_last_of('.')+1);
		if(ext == "glb" || ext == "vrm")
		{
			const auto gltf_asset = resource_system::get_file(virtual_path);
			return gltf.LoadBinaryFromMemory(&model, &error, &warning, gltf_asset.data(), gltf_asset.size());
		}
		if (ext == "gltf")
		{
			const auto base_dir = virtual_path.substr(0, virtual_path.find_last_of("\\/"));
			auto gltf_asset_text = resource_system::get_file(virtual_path);
			gltf_asset_text.push_back(0);
			const auto gltf_string = std::string((char*)gltf_asset_text.data());

			return gltf.LoadASCIIFromString(&model, &error, &warning, gltf_string.c_str(), gltf_string.size(), "");
		}

		error = "File extension is not a glTF extension for " + virtual_path;
		return false;
	}

	renderable load_mesh(const std::string& virtual_path, int index)
	{
		//TODO create model cache
		tinygltf::Model model;
		if (!load_model(virtual_path, model))
		{
			std::cerr << error;
		}

		return build_renderable(model.meshes[index], model);
	}

	renderable load_mesh(const std::string& virtual_path, const std::string& name)
	{
		//TODO create model cache
		tinygltf::Model model;
		if (!load_model(virtual_path, model))
		{
			std::cerr << error;
		}

		const auto iterator = std::find_if(std::cbegin(model.meshes), std::cend(model.meshes), 
			[&](const tinygltf::Mesh& mesh){return name == mesh.name;});

		if(iterator != std::cend(model.meshes))
		{
			return build_renderable(*iterator, model);
		}
	}

	static GLenum mode(GLenum input)
	{
		switch(input)
		{
		case TINYGLTF_MODE_POINTS:
			return GL_POINTS;
		case TINYGLTF_MODE_LINE:
			return GL_LINES;
		case TINYGLTF_MODE_LINE_LOOP:
			return GL_LINE_LOOP;
		case TINYGLTF_MODE_LINE_STRIP:
			return GL_LINE_STRIP;
		case TINYGLTF_MODE_TRIANGLES:
			return GL_TRIANGLES;
		case TINYGLTF_MODE_TRIANGLE_FAN:
			return GL_TRIANGLE_FAN;
		case TINYGLTF_MODE_TRIANGLE_STRIP:
			return GL_TRIANGLE_STRIP;
		default:
			return input;
		}
	}


	std::vector<float> get_vertices(const tinygltf::Model& model, int vertex_accessor_index, int texture_accessor_index)
	{
		std::vector<float> vertex_buffer;
		const auto vertex_accessor = model.accessors[vertex_accessor_index];
		const auto texture_accessor = model.accessors[texture_accessor_index];

		//TODO, fill the vertex buffer with xyzuv 

		return vertex_buffer;
	}

	renderable build_renderable(const tinygltf::Mesh& mesh, const tinygltf::Model& model)
	{
		std::vector<float> vertex;
		std::vector<unsigned int> index;

		//TODO, should be an array of rendeables from an array of primitives, or should combine them
		const auto primitive = mesh.primitives[0];
		const auto draw_mode = mode(primitive.mode);

		const auto indices_accessor_index = primitive.indices;
		const auto vertex_coord_accessor_index = primitive.attributes.at("POSITION");
		const auto texture_coord_accessor_index = primitive.attributes.at("TEXCOORD_0");

		renderable rd(dshader, dtexture, vertex, index, 0, 0, 0, draw_mode);
	}

};