#include "gltf_loader.hpp"
#include "resource_system.hpp"
#include <iostream>
#include <algorithm>

void gltf_loader::steal_guts(gltf_loader& loader)
{
  gltf    = loader.gltf;
  error   = std::move(loader.error);
  warning = std::move(loader.warning);
  dshader = loader.dshader;

  loader.moved_from = true;
}

gltf_loader::~gltf_loader()
{
  if(!moved_from) std::cout << "Deinitialized glTF loader\n";
}

gltf_loader::gltf_loader(shader_handle default_shader) : dshader { default_shader }
{
  std::cout << "Initialized glTF 2.0 loader using tinygltf\n";
  //This register our custom ImageLoader with tinygltf
  tinygltf_freeimage_setup(gltf);
  tinygltf_resource_system_setup(gltf);
}

gltf_loader::gltf_loader(gltf_loader&& other) noexcept { steal_guts(other); }

gltf_loader& gltf_loader::operator=(gltf_loader&& other) noexcept
{
  steal_guts(other);
  return *this;
}

bool gltf_loader::load_model(const std::string& virtual_path, tinygltf::Model& model)
{
  std::cout << "Loading gltf asset: " << virtual_path << '\n';
  const auto ext = virtual_path.substr(virtual_path.find_last_of('.') + 1);

  if(ext == "glb" || ext == "vrm")
  {
    std::cout << "Detected binary glTF\n";
    const auto gltf_asset = resource_system::get_file(virtual_path);
    return gltf.LoadBinaryFromMemory(&model, &error, &warning, gltf_asset.data(), unsigned(gltf_asset.size()));
  }

  if(ext == "gltf")
  {
    std::cout << "Detected ascii glTF\n";
    const auto base_dir = virtual_path.substr(0, 1 + virtual_path.find_last_of("\\/"));
    std::cout << "Base directory:" << base_dir << '\n';

    auto gltf_asset_text = resource_system::get_file(virtual_path);
    gltf_asset_text.push_back(0);
    const auto gltf_string = std::string(reinterpret_cast<char*>(gltf_asset_text.data()));

    return gltf.LoadASCIIFromString(
        &model, &error, &warning, gltf_string.c_str(), unsigned(gltf_string.size()), base_dir.c_str());
  }

  error = "File extension is not a glTF extension for " + virtual_path;
  std::cout << "Error:" << error << '\n';
  std::cout << "Warning:" << warning << '\n';
  return false;
}

mesh gltf_loader::load_mesh(const std::string& virtual_path, int index)
{
  error.clear();
  warning.clear();
  model_texture_cache.clear();

  //TODO create model cache
  tinygltf::Model model;
  if(!load_model(virtual_path, model)) { std::cerr << error; }

  return build_mesh(model.meshes[size_t(index)], model);
}

mesh gltf_loader::load_mesh(const std::string& virtual_path, const std::string& name)
{
  error.clear();
  warning.clear();
  model_texture_cache.clear();

  //TODO create model cache
  tinygltf::Model model;
  if(!load_model(virtual_path, model)) std::cerr << error;

  std::cerr << warning;

  const auto iterator = std::find_if(
      std::cbegin(model.meshes), std::cend(model.meshes), [&](const tinygltf::Mesh& mesh) { return name == mesh.name; });

  if(iterator != std::cend(model.meshes)) return build_mesh(*iterator, model);

  throw std::runtime_error("Couldn't find " + name + " in " + virtual_path);
}

GLenum gltf_loader::mode(GLenum input)
{
  switch(input)
  {
    case TINYGLTF_MODE_POINTS: return GL_POINTS;
    case TINYGLTF_MODE_LINE: return GL_LINES;
    case TINYGLTF_MODE_LINE_LOOP: return GL_LINE_LOOP;
    case TINYGLTF_MODE_LINE_STRIP: return GL_LINE_STRIP;
    case TINYGLTF_MODE_TRIANGLES: return GL_TRIANGLES;
    case TINYGLTF_MODE_TRIANGLE_FAN: return GL_TRIANGLE_FAN;
    case TINYGLTF_MODE_TRIANGLE_STRIP: return GL_TRIANGLE_STRIP;
    default: return input;
  }
}

std::tuple<std::vector<float>, renderable::vertex_buffer_extrema> gltf_loader::get_vertices(const tinygltf::Model& model,
                                                                                            int vertex_accessor_index,
                                                                                            int texture_accessor_index,
                                                                                            int normal_accessor_index,
                                                                                            int tangent_accessor_index)
{
  const auto c = 3 /*position*/ + 2 /*texture_coords*/ + 3 /*normal*/ + (tangent_accessor_index >= 0 ? 3 : 0);
  std::vector<float> vertex_buffer;
  const auto vertex_accessor  = model.accessors[size_t(vertex_accessor_index)];
  const auto texture_accessor = model.accessors[size_t(texture_accessor_index)];
  const auto normal_accessor  = model.accessors[size_t(normal_accessor_index)];

  // fill the vertex buffer in this manner : P[X, Y, Z] TX[U, V] N[X, Y, Z] (T [X, Y, Z])
  vertex_buffer.resize(vertex_accessor.count * (c));

  //Get the requred pointers, strides, and sizes from the accessors's buffers
  const auto vertex_buffer_view   = model.bufferViews[vertex_accessor.bufferView];
  const auto vertex_buffer_buffer = model.buffers[vertex_buffer_view.buffer];
  const auto vertex_data_start_ptr
      = vertex_buffer_buffer.data.data() + (vertex_buffer_view.byteOffset + vertex_accessor.byteOffset);
  const auto vertex_data_byte_stride = vertex_accessor.ByteStride(vertex_buffer_view);
  const auto texture_buffer_view     = model.bufferViews[texture_accessor.bufferView];
  const auto texture_buffer_buffer   = model.buffers[texture_buffer_view.buffer];
  const auto texture_data_start_ptr
      = texture_buffer_buffer.data.data() + (texture_buffer_view.byteOffset + texture_accessor.byteOffset);
  const auto texture_data_byte_stride = texture_accessor.ByteStride(texture_buffer_view);
  const auto normal_buffer_view       = model.bufferViews[normal_accessor.bufferView];
  const auto normal_buffer_buffer     = model.buffers[normal_buffer_view.buffer];
  const auto normal_data_start_ptr
      = normal_buffer_buffer.data.data() + (normal_buffer_view.byteOffset + normal_accessor.byteOffset);
  const auto normal_data_byte_stride = normal_accessor.ByteStride(normal_buffer_view);

  assert(vertex_accessor.count == texture_accessor.count);

  //We have to deal with possibly interleaved buffers that have possibliy different data types in them.
  for(size_t i = 0; i < vertex_accessor.count; i++)
  {
    //TODO check again the glTF 2.0 spec, because I think these **cannot** be double!
    //3 floats for vertex position
    for(int j = 0; j < 3; ++j) switch(vertex_accessor.componentType)
      {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          vertex_buffer[i * c + j] = ((float*)(vertex_data_start_ptr + (i * vertex_data_byte_stride)))[j];
          break;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
          vertex_buffer[i * c + j] = (float)((double*)(vertex_data_start_ptr + (i * vertex_data_byte_stride)))[j];
          break;
        default: throw std::runtime_error("unsuported vertex format");
      }

    //2 floats for vertex texture coordinates
    for(int j = 3; j < 5; ++j) switch(texture_accessor.componentType)
      {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          vertex_buffer[i * c + j] = ((float*)(texture_data_start_ptr + (i * texture_data_byte_stride)))[j - 3];
          break;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
          vertex_buffer[i * c + j] = (float)((double*)(texture_data_start_ptr + (i * texture_data_byte_stride)))[j - 3];
        default: throw std::runtime_error("unsuported vertex format");
      }

    //3 float for vertex normal (in model space)
    for(int j = 5; j < 8; ++j) switch(normal_accessor.componentType)
      {
        case TINYGLTF_COMPONENT_TYPE_FLOAT:
          vertex_buffer[i * c + j] = ((float*)(normal_data_start_ptr + (i * normal_data_byte_stride)))[j - 5];
          break;
        case TINYGLTF_COMPONENT_TYPE_DOUBLE:
          vertex_buffer[i * c + j] = (float)((double*)(normal_data_start_ptr + (i * normal_data_byte_stride)))[j - 5];
        default: throw std::runtime_error("unsuported vertex format");
      }

    if(tangent_accessor_index >= 0)
    {
      const auto& tangent_accessor      = model.accessors[size_t(tangent_accessor_index)];
      const auto& tangent_buffer_view   = model.bufferViews[tangent_accessor.bufferView];
      const auto& tangent_buffer_buffer = model.buffers[tangent_buffer_view.buffer];
      const auto tangent_data_start_ptr
          = tangent_buffer_buffer.data.data() + (tangent_buffer_view.byteOffset + tangent_accessor.byteOffset);
      const auto tangent_data_byte_stride = tangent_accessor.ByteStride(tangent_buffer_view);
      for(int j = 8; j < 11; j++) switch(tangent_accessor.componentType)
        {
          case TINYGLTF_COMPONENT_TYPE_FLOAT:
            vertex_buffer[i * c + j] = ((float*)(tangent_data_start_ptr + (i * tangent_data_byte_stride)))[j - 8];
            break;
          case TINYGLTF_COMPONENT_TYPE_DOUBLE:
            vertex_buffer[i * c + j] = (float)((double*)(tangent_data_start_ptr + (i * tangent_data_byte_stride)))[j - 8];
          default: throw std::runtime_error("unsuported vertex format");
        }
    }
  }

  glm::vec3 min { float(vertex_accessor.minValues[0]), float(vertex_accessor.minValues[1]), float(vertex_accessor.minValues[2]) };
  glm::vec3 max { float(vertex_accessor.maxValues[0]), float(vertex_accessor.maxValues[1]), float(vertex_accessor.maxValues[2]) };
  return { vertex_buffer, { min, max } };
}

std::vector<unsigned> gltf_loader::get_indices(const tinygltf::Model& model, int indices_accessor)
{
  std::vector<unsigned int> index_buffer;

  const auto index_accessor      = model.accessors[indices_accessor];
  const auto index_buffer_view   = model.bufferViews[index_accessor.bufferView];
  const auto index_buffer_buffer = model.buffers[index_buffer_view.buffer];
  const auto index_buffer_start_ptr
      = index_buffer_buffer.data.data() + (index_buffer_view.byteOffset + index_accessor.byteOffset);
  const auto index_buffer_byte_stride = index_accessor.ByteStride(index_buffer_view);

  index_buffer.resize(index_accessor.count);

  for(auto i = 0; i < index_buffer.size(); ++i) switch(index_accessor.componentType)
    {
      case TINYGLTF_COMPONENT_TYPE_INT:
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
        index_buffer[i] = ((unsigned int*)(index_buffer_start_ptr + (i * index_buffer_byte_stride)))[0];
        break;
      case TINYGLTF_COMPONENT_TYPE_SHORT:
      case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
        index_buffer[i] = ((unsigned short*)(index_buffer_start_ptr + (i * index_buffer_byte_stride)))[0];
        break;

      default: throw std::runtime_error("unreconginzed component type for index buffer");
    }

  return index_buffer;
}

GLuint gltf_loader::load_to_gl_texture(const tinygltf::Image& color_image, bool srgb) const
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

mesh gltf_loader::build_mesh(const tinygltf::Mesh& gltf_mesh, const tinygltf::Model& model)
{
  mesh output;
  const auto primitives_to_load = gltf_mesh.primitives.size();
  output.reserve_memory(primitives_to_load);
  for(int i = 0; i < primitives_to_load; ++i)
  {
    output.add_submesh([&] {
      const auto primitive = gltf_mesh.primitives[i];
      const auto draw_mode = mode(primitive.mode);

      const auto indices_accessor_index       = primitive.indices;
      const auto vertex_coord_accessor_index  = primitive.attributes.at("POSITION");
      const auto texture_coord_accessor_index = primitive.attributes.at("TEXCOORD_0");
      const auto normal_accessor_index        = primitive.attributes.at("NORMAL");
      const auto tangent_accessor_index_it    = primitive.attributes.find("TANGENT");
      const auto tangent_accessor_index
          = tangent_accessor_index_it != primitive.attributes.end() ? tangent_accessor_index_it->second : -1;

      auto [vertex, aabb] = get_vertices(
          model, vertex_coord_accessor_index, texture_coord_accessor_index, normal_accessor_index, tangent_accessor_index);
      auto index = get_indices(model, indices_accessor_index);

      if(gltf_mesh.primitives[i].material >= 0)
      {

        texture_handle diffuse_texture_handle = texture_manager::invalid_texture;
        texture_handle normal_texture_handle  = texture_manager::invalid_texture;

        const auto material     = model.materials[gltf_mesh.primitives[i].material];
        int color_texture_index = -1;
        auto color_texture_it   = material.values.find("baseColorTexture");
        if(color_texture_it != material.values.end()) color_texture_index = color_texture_it->second.TextureIndex();
        if(color_texture_index >= 0)
        {
          const auto color_texture = model.textures[material.values.at("baseColorTexture").TextureIndex()];
          const auto color_image   = model.images[color_texture.source];

          {
            const auto cached = model_texture_cache.find(color_texture.source);
            if(cached == std::end(model_texture_cache))
            {
              GLuint tex             = load_to_gl_texture(color_image);
              diffuse_texture_handle = texture_manager::create_texture(tex);
              {
                auto& diffuse_texture_object = texture_manager::get_from_handle(diffuse_texture_handle);
                diffuse_texture_object.generate_mipmaps();
                diffuse_texture_object.set_filtering_parameters();
              }
              model_texture_cache[color_texture.source] = diffuse_texture_handle;
            }
            else
            {
              diffuse_texture_handle = cached->second;
            }
          }
        }

        int normal_texture_index = -1;
        {
          auto normal_texture_it = material.additionalValues.find("normalTexture");
          if(normal_texture_it != material.additionalValues.end())
            normal_texture_index = normal_texture_it->second.TextureIndex();
        }

        if(normal_texture_index >= 0)
        {
          const auto normal_texture = model.textures[normal_texture_index];
          const auto normal_image   = model.images[normal_texture.source];

          const auto cached = model_texture_cache.find(normal_texture.source);
          if(cached == std::end(model_texture_cache))
          {
            GLuint tex            = load_to_gl_texture(normal_image, false);
            normal_texture_handle = texture_manager::create_texture(tex);
            {
              auto& normal_texture_object = texture_manager::get_from_handle(normal_texture_handle);
              normal_texture_object.generate_mipmaps();
              normal_texture_object.set_filtering_parameters();
            }
            model_texture_cache[normal_texture.source] = normal_texture_handle;
          }
          else
          {
            normal_texture_handle = cached->second;
          }
        }

        renderable_handle r
            = renderable_manager::create_renderable(dshader,
                                                    vertex,
                                                    index,
                                                    aabb,
                                                    renderable::configuration { true, true, true, tangent_accessor_index >= 0 },
                                                    tangent_accessor_index >= 0 ? 11 : 8,
                                                    0,
                                                    3,
                                                    5,
                                                    8,
                                                    draw_mode);
        {
          auto& renderable = renderable_manager::get_from_handle(r);
          renderable.set_diffuse_texture(diffuse_texture_handle);
          renderable.set_normal_texture(normal_texture_handle);
        }

        return r;
      }

      auto r = renderable_manager::create_renderable(dshader,
                                                     vertex,
                                                     index,
                                                     aabb,
                                                     renderable::configuration { true, true, true, tangent_accessor_index >= 0 },
                                                     tangent_accessor_index >= 0 ? 11 : 8,
                                                     0,
                                                     3,
                                                     5,
                                                     8,
                                                     draw_mode);
      return r;
    }());
  }

  return output;
}
