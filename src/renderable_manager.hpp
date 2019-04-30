#pragma once

#include "renderable.hpp"
#include <vector>

using renderable_handle = std::vector<renderable>::size_type;

class renderable_manager
{
	static renderable_manager* manager;
	std::vector<renderable> renderables;
	std::vector<renderable_handle> unallocated_renderables;

public:
	static constexpr renderable_handle invalid_renderable { std::numeric_limits<renderable_handle>::max() };

	renderable_manager();
	~renderable_manager();

	static renderable& get_from_handle(renderable_handle r);
	static void get_rid_of(renderable_handle r);

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

	static std::vector<renderable> const& get_list() { return manager->renderables; }
};
