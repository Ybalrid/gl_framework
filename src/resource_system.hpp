#pragma once

#include <string>
#include <vector>
#include "physfs_raii.hpp"

class resource_system
{
	physfs_raii pysics_fs;

public:
	explicit resource_system(char* arg0);
	~resource_system();
	static void add_location(const std::string& real_path, bool last = true);
	static std::vector<uint8_t> get_file(const std::string& virtual_path);
};
