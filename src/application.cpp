#pragma once
#include "application.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "renderable.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include "camera.hpp"

#include "gui.hpp"

std::vector<std::string> application::resource_paks;

void application::activate_vsync()
{
	try
	{
		sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::adaptive_vsync);
	}
	catch (const sdl::Exception& e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << "Using standard vsync instead of adaptive vsync\n";
		try
		{
			sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::vsync);
		}
		catch (const sdl::Exception& e)
		{
			std::cerr << e.what() << '\n';
			std::cerr << "Cannot set vsync for this driver.\n";
			try
			{
				sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);
			}
			catch (const sdl::Exception& e)
			{
				std::cerr << e.what() << '\n';
			}
		}
	}
}

void application::handle_event(const sdl::Event& e)
{
}

application::application(int argc, char** argv) : resources(argc > 0 ? argv[0] : nullptr)
{
	FreeImage_Initialise(); //RAII this!!

	//init sdl
	auto root = sdl::Root(SDL_INIT_EVERYTHING);
	sdl::Event event{};

	//create window
	auto window = sdl::Window("application window",
		{ 800, 600 }, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);

	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); //OpenGL 4+
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 6); //OpenGL 4.6

	//create OpenGL context
	auto context = window.create_context();
	context.make_current();

	if (glewInit() != GLEW_OK)
	{
		std::cerr << "cannot init glew\n";
		abort();
	}

	gui ui(window, context);

	std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
	//set vsync mode
	activate_vsync();

	for (const auto pak : resource_paks)
	{
		std::cerr << "adding to resources " << pak << '\n';
		resource_system::add_location(pak);
	}

	texture polutropon_logo_texture;
	{
		auto img = image("/polutropon.png");
		polutropon_logo_texture.load_from(img);
		polutropon_logo_texture.generate_mipmaps();
	}

	std::vector<float> plane = {
	//	 x=		 y=		z=		u=	v=
		-0.9f,	0.0f,	 0.9f,		0,	0,
		 0.9f,	0.0f,	 0.9f,		1,	0,
		-0.9f,	0.0f,	-0.9f,		0,	1,
		 0.9f,	0.0f,	-0.9f,		1,	1
	};

	std::vector<unsigned int> plane_indices =
	{
		0, 1, 2, //triangle 0
		1, 3, 2  //triangle 1
	};

	shader simple_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
	renderable textured_plane(simple_shader, polutropon_logo_texture, plane, plane_indices, 3 + 2, 0, 3);
	//set opengl clear color
	glClearColor(0.2f, 0.3f, 0.4f, 1);

	auto tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 2.f, 2.f));
	auto rot = glm::rotate(glm::mat4(1.f), glm::radians(45.f), glm::vec3(-1.f, 0, 0));
	
	camera cam;
	cam.set_model(tr*rot);

	//view = glm::transpose(view);
	//main loop
	glEnable(GL_DEPTH_TEST);
	while (running)
	{
		//event polling
		while (event.poll())
		{
			ui.handle_event(event);
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
			default:
				handle_event(event);
				break;
			}
		}

		ui.frame();
		const auto size = window.size();
		cam.update_projection(size.x, size.y);
		
		glm::mat4 model = glm::rotate(glm::mat4(1.f), glm::radians(45.f * (float(SDL_GetTicks())/1000.f)), glm::vec3(0, 1.f, 0));
		model = glm::scale(model, (2 + glm::sin(SDL_GetTicks() / 1000.f)) * glm::vec3(0.5f) );

		//clear viewport
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);

		textured_plane.set_mvp_matrix(cam.view_porjection_matrix() * model);
		textured_plane.draw();

		model = glm::translate(model, glm::vec3(2.f, 0.f, 0));
		textured_plane.set_mvp_matrix(cam.view_porjection_matrix() * model);
		textured_plane.draw();

		model = glm::translate(model, glm::vec3(-4.f, 0.f, 0));
		textured_plane.set_mvp_matrix(cam.view_porjection_matrix() * model);
		textured_plane.draw();

		ImGui::ShowDemoWindow();

		ui.render();

		//swap buffers
		window.gl_swap();
	}

	FreeImage_DeInitialise();
}
