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

	shader* dshader = nullptr;

	//TODO don't store that here.
	std::vector<texture> gltf_textures{};

	void steal_guts(gltf_loader& loader)
	{
		gltf = loader.gltf;
		error = std::move(loader.error);
		warning = std::move(loader.warning);

		dshader = loader.dshader;

		gltf_textures = std::move(loader.gltf_textures);
	}

public:

	gltf_loader() = default;
	~gltf_loader() = default;

	gltf_loader(shader& default_shader) : dshader{ &default_shader }
	{
		std::cout << "Initialized glTF 2.0 loader using tinygltf\n";
		gltf_textures.reserve(10);
		//This register our custom ImageLoader with tinygltf
		tinygltf_freeimage_setup(gltf);
		tinygltf_resource_system_setup(gltf);
	}

	gltf_loader(const gltf_loader&) = delete;
	gltf_loader& operator=(const gltf_loader&) = delete;
	gltf_loader(gltf_loader&& other) noexcept
	{
		steal_guts(other);
	}
	gltf_loader& operator=(gltf_loader&& other) noexcept
	{
		steal_guts(other);
		return *this;
	}

	bool load_model(const std::string& virtual_path, tinygltf::Model& model)
	{
		const auto ext = virtual_path.substr(virtual_path.find_last_of('.')+1);
		if(ext == "glb" || ext == "vrm")
		{
			const auto gltf_asset = resource_system::get_file(virtual_path);
			return gltf.LoadBinaryFromMemory(&model, &error, &warning, gltf_asset.data(), unsigned(gltf_asset.size()));
		}
		if (ext == "gltf")
		{
			const auto base_dir = virtual_path.substr(0, virtual_path.find_last_of("\\/"));
			auto gltf_asset_text = resource_system::get_file(virtual_path);
			gltf_asset_text.push_back(0);
			const auto gltf_string = std::string(reinterpret_cast<char*>(gltf_asset_text.data()));

			return gltf.LoadASCIIFromString(&model, &error, &warning, gltf_string.c_str(), unsigned(gltf_string.size()), "");
		}

		error = "File extension is not a glTF extension for " + virtual_path;
		return false;
	}

	renderable load_mesh(const std::string& virtual_path, int index)
	{
		error.clear();
		warning.clear();

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
		error.clear();
		warning.clear();

		//TODO create model cache
		tinygltf::Model model;
		if (!load_model(virtual_path, model))
			std::cerr << error;

		std::cerr << warning;

		const auto iterator = std::find_if(std::cbegin(model.meshes), std::cend(model.meshes), 
			[&](const tinygltf::Mesh& mesh){return name == mesh.name;});

		if(iterator != std::cend(model.meshes))
			return build_renderable(*iterator, model);

		throw std::runtime_error("Couldn't find " + name + " in " + virtual_path);
	}

	//this is a bit unnecessary for OpenGL as theses identifier are the same
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

	static std::vector<float> get_vertices(const tinygltf::Model& model, int vertex_accessor_index, int texture_accessor_index, int normal_accessor_index)
	{
		const auto c = 3 /*position*/ + 2/*texture_coords*/ + 3/*normal*/;
		std::vector<float> vertex_buffer;
		const auto vertex_accessor = model.accessors[vertex_accessor_index];
		const auto texture_accessor = model.accessors[texture_accessor_index];
		const auto normal_accessor = model.accessors[normal_accessor_index];

		// fill the vertex buffer in this manner : P[X, Y, Z] TX[U, V] N[X, Y, Z]
		vertex_buffer.resize(vertex_accessor.count * (c));

		//Get the requred pointers, strides, and sizes from the accessors's buffers
		const auto vertex_buffer_view = model.bufferViews[vertex_accessor.bufferView];
		const auto vertex_buffer_buffer = model.buffers[vertex_buffer_view.buffer];
		const auto vertex_data_start_ptr = vertex_buffer_buffer.data.data() + (vertex_buffer_view.byteOffset + vertex_accessor.byteOffset);
		const auto vertex_data_byte_stride = vertex_accessor.ByteStride(vertex_buffer_view);
		const auto texture_buffer_view = model.bufferViews[texture_accessor.bufferView];
		const auto texture_buffer_buffer = model.buffers[texture_buffer_view.buffer];
		const auto texture_data_start_ptr = texture_buffer_buffer.data.data() + (texture_buffer_view.byteOffset + texture_accessor.byteOffset);
		const auto texture_data_byte_stride = texture_accessor.ByteStride(texture_buffer_view);
		const auto normal_buffer_view = model.bufferViews[normal_accessor.bufferView];
		const auto normal_buffer_buffer = model.buffers[normal_buffer_view.buffer];
		const auto normal_data_start_ptr = normal_buffer_buffer.data.data() + (normal_buffer_view.byteOffset + normal_accessor.byteOffset);
		const auto normal_data_byte_stride = normal_accessor.ByteStride(normal_buffer_view);

		assert(vertex_accessor.count == texture_accessor.count);

		//We have to deal with possibly interleaved buffers that have possibliy different data types in them.
		for (auto i = 0; i < vertex_accessor.count; i++)
		{	
			//3 floats for vertex position
			for(int j = 0; j < 3; ++j)	switch(vertex_accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				vertex_buffer[i * c + j] = ((float*)(vertex_data_start_ptr + (i*vertex_data_byte_stride )))[j];
				break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
				vertex_buffer[i * c + j] = (float)((double*)(vertex_data_start_ptr + (i*vertex_data_byte_stride )))[j];
				break;
			default: throw std::runtime_error("unsuported vertex format");
			}

			//2 floats for vertex texture coordinates
			for(int j = 3; j < 5; ++j) switch(texture_accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				vertex_buffer[i * c + j] = ((float*)(texture_data_start_ptr+(i*texture_data_byte_stride)))[j-3];
				break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
				vertex_buffer[i * c + j] = (float)((double*)(texture_data_start_ptr+(i*texture_data_byte_stride)))[j-3];
			default: throw std::runtime_error("unsuported vertex format");
				
			}

			//3 float for vertex normal (in model space)
			for(int j = 5; j < 8; ++j) switch(normal_accessor.componentType)
			{
			case TINYGLTF_COMPONENT_TYPE_FLOAT:
				vertex_buffer[i * c + j] = ((float*)(normal_data_start_ptr+(i*normal_data_byte_stride)))[j-5];
				break;
			case TINYGLTF_COMPONENT_TYPE_DOUBLE:
				vertex_buffer[i * c + j] = (float)((double*)(normal_data_start_ptr+(i*normal_data_byte_stride)))[j-5];
			default: throw std::runtime_error("unsuported vertex format");
				
			}
		}
		
		return vertex_buffer;
	}

	static std::vector<unsigned int> get_indices(const tinygltf::Model& model, int indices_accessor)
	{
		std::vector<unsigned int> index_buffer;

		const auto index_accessor = model.accessors[indices_accessor];
		const auto index_buffer_view = model.bufferViews[index_accessor.bufferView];
		const auto index_buffer_buffer = model.buffers[index_buffer_view.buffer];
		const auto index_buffer_start_ptr = index_buffer_buffer.data.data() + (index_buffer_view.byteOffset + index_accessor.byteOffset);
		const auto index_buffer_byte_stride = index_accessor.ByteStride(index_buffer_view);

		index_buffer.resize(index_accessor.count);

		for(auto i = 0; i < index_buffer.size(); ++i) switch(index_accessor.componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_INT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
			index_buffer[i] = ((unsigned int*)(index_buffer_start_ptr+(i*index_buffer_byte_stride)))[0];
			break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			index_buffer[i] = ((unsigned short*)(index_buffer_start_ptr+(i*index_buffer_byte_stride)))[0];
			break;

		default: throw std::runtime_error("unreconginzed component type for index buffer");
		}

		return index_buffer;
	}

	GLuint load_to_gl_texture(const tinygltf::Image& color_image, bool srgb = true)
	{
		GLuint tex;
		glGenTextures(1, &tex);
		glBindTexture(GL_TEXTURE_2D, tex);
		glTexImage2D(GL_TEXTURE_2D, 
		             0,
		             srgb ? GL_SRGB_ALPHA : GL_RGBA,
		             color_image.width, 
		             color_image.height, 
		             0,
		             GL_RGBA, 
		             GL_UNSIGNED_BYTE, 
		             color_image.image.data());
		glGenerateMipmap(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, 0);

		return tex;
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
		const auto normal_accessor_index = primitive.attributes.at("NORMAL");

		vertex = get_vertices(model, vertex_coord_accessor_index, texture_coord_accessor_index, normal_accessor_index);
		index = get_indices(model, indices_accessor_index);

		if(mesh.primitives[0].material>=0)
		{

			texture* diffuse_texture = nullptr;
			const auto material = model.materials[mesh.primitives[0].material];
			const auto color_texture = model.textures[material.values.at("baseColorTexture").TextureIndex()];
			const auto color_image = model.images[color_texture.source];

			GLuint tex = load_to_gl_texture(color_image);


			gltf_textures.emplace_back(tex);
			diffuse_texture = &gltf_textures.back();
			

			renderable r{ dshader, vertex, index,{true, true, true} , 8, 0, 3, 5, draw_mode };
			r.set_diffuse_texture(diffuse_texture);

			return r;
		}

		renderable r{ dshader, vertex, index, {true, true, true}, 8, 0, 3, 5, draw_mode };

		return r;
	}

};
