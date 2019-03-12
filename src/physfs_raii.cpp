#include "physfs_raii.hpp"
#include "physfs.h"
#include <iostream>

physfs_raii::physfs_raii(char* argv0)
{
	PHYSFS_init(argv0);
	init = true;
	PHYSFS_Version v;
	PHYSFS_getLinkedVersion(&v);
	std::cout << "Initialized Physics_FS version " << int(v.major) << '.' << int(v.minor) << '.' << int(v.patch) << '\n';
}

physfs_raii::~physfs_raii()
{
	if (init)
	{
		PHYSFS_deinit();
		std::cout << "Deinitialized Physics_FS\n";
	}
}

physfs_raii::physfs_raii(physfs_raii&& other) noexcept
{
	steal_guts(other);
}

physfs_raii& physfs_raii::operator=(physfs_raii&& other) noexcept
{
	steal_guts(other);
	return *this;
}

void physfs_raii::steal_guts(physfs_raii& other)
{
	init	   = other.init;
	other.init = false;
}
