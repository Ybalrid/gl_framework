#include "application.hpp"
#include <iostream>

#ifdef __linux__
#include <unistd.h>
#endif

int main(int argc, char* argv[])
{
	std::cout << "my pid is : " << getpid() << '\n';
	application::resource_paks.emplace_back("./res.zip");
	application::resource_paks.emplace_back("./unpacked_res/");
	application a(argc, argv);
	a.run();

	return 0;
}
