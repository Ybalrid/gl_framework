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

/*
 * TODO:
 *  - Build node hierarchy that mimic the one inside a glTF scene
 *  - Load every single meshes on that scene and attach them to the nodes
 */

///Function calls that setup filesystem and image decoding callback to tinygltf
void tinygltf_freeimage_setup(tinygltf::TinyGLTF& gltf);
void tinygltf_resource_system_setup(tinygltf::TinyGLTF& gltf);

class gltf_loader
{
  ///TinyGLTF state
  tinygltf::TinyGLTF gltf;
  std::string error;
  std::string warning;

  ///Default shader
  shader_handle dshader = shader_program_manager::invalid_shader;

  ///Move utility
  void steal_guts(gltf_loader& loader);
  bool moved_from = false;

  ///Textures from the currently loading model
  std::unordered_map<int, texture_handle> model_texture_cache;

  public:
  gltf_loader() = default;
  ~gltf_loader();

  ///Usual constructor
  gltf_loader(shader_handle default_shader);

  ///No copy
  gltf_loader(const gltf_loader&) = delete;
  ///No copy
  gltf_loader& operator=(const gltf_loader&) = delete;

  ///Explicit move
  gltf_loader(gltf_loader&& other) noexcept;
  ///Explicit move
  gltf_loader& operator=(gltf_loader&& other) noexcept;

  ///Load a full model
  bool load_model(const std::string& virtual_path, tinygltf::Model& model);
  ///Load one mesh
  mesh load_mesh(const std::string& virtual_path, int index);
  ///Load one mesh
  mesh load_mesh(const std::string& virtual_path, const std::string& name);

  ///Return an OpenGL display operation mode from a glTF display mode
  static GLenum mode(GLenum input);
  ///Load vertex buffer
  static std::tuple<std::vector<float>, renderable::vertex_buffer_extrema> get_vertices(const tinygltf::Model& model,
                                                                                        int vertex_accessor_index,
                                                                                        int texture_accessor_index,
                                                                                        int normal_accessor_index,
                                                                                        int tangent_accessor_index);
  ///Get index buffer
  static std::vector<unsigned int> get_indices(const tinygltf::Model& model, int indices_accessor);

  ///Load tinygltf image to an OpenGL texture
  GLuint load_to_gl_texture(const tinygltf::Image& color_image, bool srgb = true) const;

  ///Construct a mesh from a glTF mesh.
  mesh build_mesh(const tinygltf::Mesh& gltf_mesh, const tinygltf::Model& model);
};
