#pragma once

#include <string>
#include <vector>
#include "physfs_raii.hpp"

///PhysicsFS based resource system
class resource_system
{
  ///RAII physicsfs
  physfs_raii pysics_fs;

  public:
  ///Setup the system, use args[0]!
  explicit resource_system(char* arg0);

  ///Cleanup
  ~resource_system();

  ///Mount a location to the virtual file system
  static void add_location(const std::string& real_path, bool last = true);

  ///Get an array of bytes in memory that correspond to that virtual file
  static std::vector<uint8_t> get_file(const std::string& virtual_path);

  ///List the files in the virtual file system
  static std::vector<std::string> list_files(const std::string& root = "/", bool recursive = false);
};
