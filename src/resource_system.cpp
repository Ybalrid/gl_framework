#include "resource_system.hpp"

resource_system::resource_system(char* arg0): pysics_fs(arg0)
{
}

void resource_system::add_location(const std::string& real_path, bool last)
{
	PHYSFS_addToSearchPath(real_path.c_str(), last ? 1 : 0);
}

std::vector<uint8_t> resource_system::get_file(const std::string& virtual_path)
{
	if (!PHYSFS_exists(virtual_path.c_str())) 
		throw std::runtime_error("file "+ virtual_path +" doesn't exist");

	const auto file = PHYSFS_openRead(virtual_path.c_str());

	if (!file)
		throw std::runtime_error("could not open " + virtual_path + "for reading");

	const auto size = PHYSFS_fileLength(file);
	std::vector<uint8_t> data(size);
	PHYSFS_read(file, data.data(), 1, size);
	PHYSFS_close(file);

	return data; //rvo ~o~
}
