#include "application.hpp"

int main(int argc, char* argv[])
{
	application::resource_paks.emplace_back("./res.zip");
	application::resource_paks.emplace_back("./unpakced_res/");
	application a(argc, argv);
	a.run();

	return 0;
}
