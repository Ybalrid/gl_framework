#pragma once
#include "physfs.h"
#include <iostream>

struct physfs_raii
{
	bool init = false;

	explicit physfs_raii(char* argv0)
	{
		PHYSFS_init(argv0);
		init = true;
		PHYSFS_Version v;
		PHYSFS_getLinkedVersion(&v);
		std::cout << "Initialized Physics_FS version " << int(v.major) << '.' << int(v.minor) << '.' << int(v.patch) << '\n';
	}

	~physfs_raii()
	{
		if (init) PHYSFS_deinit();
	}

	physfs_raii(const physfs_raii&) = delete;
	physfs_raii& operator=(const physfs_raii&) = delete;

	physfs_raii(physfs_raii&& other) noexcept
	{
		steal_guts(other);
	}

	physfs_raii& operator=(physfs_raii&& other) noexcept
	{
		steal_guts(other);
		return *this;
	}

private:
	void steal_guts(physfs_raii& other)
	{
		init = other.init;
		other.init = false;
	}
};
