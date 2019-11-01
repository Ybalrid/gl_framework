#pragma once

#include "renderable.hpp"
#include <vector>

///Define the type of an handle
using renderable_handle = std::vector<renderable>::size_type;

///Keep track of renderables allocated by the engine
class renderable_manager
{
  ///Singleton pointer
  static renderable_manager* manager;

  ///List of renderables
  std::vector<renderable> renderables;
  ///List of renderable handles
  std::vector<renderable_handle> unallocated_renderables;

  public:
  ///Biggest number possible = invalid index
  static constexpr renderable_handle invalid_renderable { std::numeric_limits<renderable_handle>::max() };

  renderable_manager();
  ~renderable_manager();

  ///Get access to a renderable from it's handle
  static renderable& get_from_handle(renderable_handle r);
  ///Destroy the resource
  static void get_rid_of(renderable_handle r);

  ///Construct a renderable inside the manager
  template <typename... Args>
  static renderable_handle create_renderable(Args... args)
  {
    if(manager->unallocated_renderables.empty())
    {
      manager->renderables.emplace_back(args...);
      return manager->renderables.size() - 1;
    }

    const auto handle = manager->unallocated_renderables.back();
    manager->unallocated_renderables.pop_back();
    manager->get_from_handle(handle) = std::move(renderable(args...));
    return handle;
  }

  ///Get list or renderables
  static std::vector<renderable> const& get_list();
};
