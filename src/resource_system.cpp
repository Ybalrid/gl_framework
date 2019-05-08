#include "resource_system.hpp"
#include <iostream>
#include <physfs.h>
#include <algorithm>

resource_system::resource_system(char* arg0) : pysics_fs(arg0)
{
	std::cout << "Initialized Resource system"
#ifdef VERBOSE
				 " for "
			  << arg0 << '\n'
#endif
			  << '\n';
#ifdef VERBOSE
	for(auto i = PHYSFS_supportedArchiveTypes(); *i != NULL; i++)
	{ printf("Supported archive: [%s], which is [%s].\n", (*i)->extension, (*i)->description); }
#endif
}

resource_system::~resource_system() { std::cout << "Deinitialized Resource system\n"; }

void resource_system::add_location(const std::string& real_path, bool last)
{
	PHYSFS_mount(real_path.c_str(), nullptr, (last ? 1 : 0));
}

std::vector<uint8_t> resource_system::get_file(const std::string& virtual_path)
{
	if(!PHYSFS_exists(virtual_path.c_str())) throw std::runtime_error("file " + virtual_path + " doesn't exist");

	const auto file = PHYSFS_openRead(virtual_path.c_str());

	if(!file) throw std::runtime_error("could not open " + virtual_path + "for reading");

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
	const auto list = PHYSFS_enumerateFiles(root.c_str());								//returns an array of char*
	for(auto strptr = list; *strptr != nullptr; strptr++) output.emplace_back(*strptr); //Construct std::strings on the fly
	PHYSFS_freeList(list);																//Do not forget to free the array!

	//Recurse on subdirectories if this is true
	if(recursive)
	{
		//Cannot use an iterator that would be invalidated, iterate by index
		for(size_t i = 0; i < output.size(); ++i)
		{
			//Get full path of (potential) subdirectories
			const auto file = output[i];
			auto path		= root;
			if(path[path.size() - 1] != '/') path += "/"; //In case root doesn't end with "/"
			path += file; //This file may, or may not be a directory. path is a full path form root to it

			//Check if file is a directory
			if(PHYSFS_stat(path.c_str(), &stat) != 0 && stat.filetype == PHYSFS_FILETYPE_DIRECTORY)
			{
				//Append to the output all the files from the subdirectory
				const auto files_in_subdir = list_files(path, true);
				for(const auto& file_in_subdir : files_in_subdir)
				{
					output.push_back(path + file_in_subdir); //as path starting from root
				}
			}
		}
	}

	//Adjust files to make them valid path, and to make all directory names end with /
	for(auto& file : output)
	{
		if(file[0] != '/') file = "/" + file;

		if(PHYSFS_stat(file.c_str(), &stat) != 0 && stat.filetype == PHYSFS_FILETYPE_DIRECTORY) file += "/";
	}

	//Sort them
	std::sort(output.begin(), output.end());
	return output; //should benefit from rvo
}
