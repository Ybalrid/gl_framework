//Just start an application
#include "application.hpp"

int main(int argc, char* argv[])
{
	application::resource_paks.emplace_back("./res.zip");
	application::resource_paks.emplace_back("./unpacked_res/");
	application a(argc, argv, "application");
	a.run();

	return 0;
}
