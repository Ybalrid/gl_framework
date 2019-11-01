#pragma once

#include "texture.hpp"

using texture_handle = std::vector<texture>::size_type;

class texture_manager
{
  static texture_manager* manager;
  static texture_handle dummy_texture;
  std::vector<texture> textures;
  std::vector<texture_handle> unallocated_textures;

  public:
  static constexpr texture_handle invalid_texture = { std::numeric_limits<texture_handle>::max() };
  texture_manager();
  ~texture_manager();

  static texture& get_from_handle(texture_handle t);
  static void get_rid_of(texture_handle t);

  static void initialize_dummy_texture();
  static texture_handle get_dummy_texture() { return dummy_texture; }

  template <typename... ConstructorArgs>
  static texture_handle create_texture(ConstructorArgs... args)
  {
    if(manager->unallocated_textures.empty())
    {
      manager->textures.emplace_back(args...);
      return manager->textures.size() - 1;
    }

    const texture_handle handle = manager->unallocated_textures.back();
    manager->unallocated_textures.pop_back();
    get_from_handle(handle) = texture(args...);
    return handle;
  }

  static std::vector<texture> const& get_list() { return manager->textures; }
};
