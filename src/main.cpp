#include <GL/glew.h>
#define CPP_SDL2_GL_WINDOW
#include <cpp-sdl2/sdl.hpp>

#include <iostream>

int main(int argc, char* argv[])
{
	(void)(argc);
	(void)(argv);

	//init sdl
	auto root = sdl::Root(SDL_INIT_EVERYTHING);
	sdl::Event event;

	//create window
	auto window = sdl::Window("application window",
			{1024,768}, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	//create opengl context
	auto context = window.create_context();
	context.make_current();
	
	if(glewInit() != GLEW_OK)
	{
		std::cerr << "cannot init glew\n";
		abort();
	}


	std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
	//set vsync mode
	try
	{
		window.gl_set_swap_interval(sdl::Window::gl_swap_interval::adaptive_vsync);
	}
	catch (sdl::Exception e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << "Using standard vsync instead of adaptive vsync\n";
		try
		{
			window.gl_set_swap_interval(sdl::Window::gl_swap_interval::vsync);
		}
		catch(sdl::Exception e)
		{
			std::cerr << e.what() << '\n';
			std::cerr << "Cannot set vsync for this driver.\n";
			try
			{
				window.gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);
			}
			catch(sdl::Exception e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	}

	//set opengl clear color
	glClearColor(0.2f, 0.3f, 0.4f, 1);

	//main loop
	auto run = true;
	while(run)
	{
		//event polling
		while(event.poll())
		{
			switch(event.type)
			{
				case SDL_QUIT:
					run = false;
			}
		}

		//clear viewport
		glClear(GL_COLOR_BUFFER_BIT);


		//swap buffers
		window.gl_swap();
	}

	return 0;
}

