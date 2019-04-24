#include "resource_system.hpp"
#include <iostream>
#include <physfs.h>
#include <algorithm>

resource_system::resource_system(char* arg0) :
 pysics_fs(arg0)
{
	std::cout << "Initialized Resource system"
#ifdef VERBOSE
				 " for "
			  << arg0 << '\n'
#endif
			  << '\n';
#ifdef VERBOSE
	for(auto i = PHYSFS_supportedArchiveTypes(); *i != NULL; i++)
	{
		printf("Supported archive: [%s], which is [%s].\n",
			   (*i)->extension,
			   (*i)->description);
	}
#endif
}

resource_system::~resource_system()
{
	std::cout << "Deinitialized Resource system\n";
}

void resource_system::add_location(const std::string& real_path, bool last)
{
	PHYSFS_mount(real_path.c_str(), nullptr, (last ? 1 : 0));
}

std::vector<uint8_t> resource_system::get_file(const std::string& virtual_path)
{
	if(!PHYSFS_exists(virtual_path.c_str()))
		throw std::runtime_error("file " + virtual_path + " doesn't exist");

	const auto file = PHYSFS_openRead(virtual_path.c_str());

	if(!file)
		throw std::runtime_error("could not open " + virtual_path + "for reading");

	const auto size = PHYSFS_fileLength(file);
	std::vector<uint8_t> data(size);
	PHYSFS_readBytes(file, data.data(), size);
	PHYSFS_close(file);

	return data; //rvo ~o~
}

std::vector<std::string> resource_system::list_files(const std::string& root, bool recursive)
{
	std::vector<std::string> output;
	PHYSFS_Stat stat;

	//List files
	const auto list = PHYSFS_enumerateFiles(root.c_str());
	for(auto strptr = list; *strptr != nullptr; strptr++)
		output.emplace_back(*strptr);
	PHYSFS_freeList(list);

	//Recurse on subdirectories if this is true
	if(recursive)
	{
		//Cannot use an iterator that would be invalidated, iterate by index
		for(size_t i = 0; i < output.size(); ++i)
		{
			//Get full path of (pontential) subdirectory
			const auto file = output[i];
			auto path		= root;
			if(path[path.size() - 1] != '/') path += "/";
			path += file;

			//Check if file is a directory
			if(PHYSFS_stat(path.c_str(), &stat) != 0 && stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
			{
				//append to the output all the files from the subdirectory
				const auto recursed = list_files(path, true);
				for(const auto& recursed_file : recursed)
				{
					output.push_back(path + recursed_file); //as path starting from root
				}
			}
		}
	}

	//Adjust files to make them valid path, and to make all directory names end with /
	for(auto& file : output)
	{
		if(file[0] != '/') file = "/" + file;

		if(PHYSFS_stat(file.c_str(), &stat) != 0 && stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
			file += "/";
	}

	//Sort them
	std::sort(output.begin(), output.end());
	return output;
}
