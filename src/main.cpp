#include "application.hpp"

int main(int argc, char* argv[])
{

	application::resource_paks.emplace_back("./res.zip");
	application a(argc, argv);

	return 0;
}

