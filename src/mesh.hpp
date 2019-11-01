#pragma once

#include <vector>
#include "renderable_manager.hpp"

class mesh
{
  std::vector<renderable_handle> submeshes {};

  public:
  void add_submesh(renderable_handle primitive) { submeshes.push_back(primitive); }
  std::vector<renderable_handle> const& get_submeshes() const { return submeshes; }
  void reserve_memory(size_t elem_count)
  {
    if(elem_count > submeshes.size()) submeshes.reserve(elem_count);
  }
};
