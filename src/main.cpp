//Just start an application
#include "application.hpp"
#include "build_config.hpp"

int main(int argc, char* argv[])
{
  application::resource_paks.emplace_back("./res.zip");
  application::resource_paks.emplace_back("./unpacked_res/");
  application a(argc, argv, GAME_NAME);
  a.run();

  return 0;
}
