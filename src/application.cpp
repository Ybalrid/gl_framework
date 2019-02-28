#pragma once
#include "application.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "renderable.hpp"

#include <glm/gtc/matrix_transform.hpp>
#include "camera.hpp"

#include "gui.hpp"
#include "scene_object.hpp"
#include "gltf_loader.hpp"

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
	switch (e.type)
	{
	case SDL_KEYDOWN:
		if(!e.key.repeat)
			switch(e.key.keysym.sym)
			{
			case SDLK_TAB:
				debug_ui = !debug_ui;
				break;
			default:break;
			}
		break;
	case SDL_KEYUP:
		if(!e.key.repeat)
			switch(e.key.keysym.sym)
			{
			default:break;
			}
		break;
	}
}

void application::draw_debug_ui()
{
	if (debug_ui)
	{
		ImGui::Begin("Debug Window", &debug_ui);
		ImGui::Text("Debug information");
		ImGui::Text("FPS: %d", fps);
		ImGui::End();
	}
}

void application::animate(scene_object& plane0, scene_object& plane1, scene_object& plane2)
{
	return;
	glm::mat4 model = glm::rotate(glm::mat4(1.f), glm::radians(45.f * (float(SDL_GetTicks())/1000.f)), glm::vec3(0, 1.f, 0));
	model = glm::scale(model, (2 + glm::sin(SDL_GetTicks() / 1000.f)) * glm::vec3(0.5f) );
	plane0.set_model(model);
	model = glm::translate(model, glm::vec3(2.f, 0.f, 0));
	plane1.set_model(model);
	model = glm::translate(model, glm::vec3(-4.f, 0.f, 0));
	plane2.set_model(model);
}

void application::update_timing()
{
	last_frame_time = current_time;
	current_time = SDL_GetTicks();
	if (current_time - last_second_time >= 1000)
	{
		fps = frames;
		frames = 0;
		last_second_time = current_time;
	}
	frames++;
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

	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param)
	{
		std::cerr << "-----\n";
		std::cerr << "opengl debug message: " << glGetString(source) << ' ' << glGetString(type) << ' ' << id << ' ' << std::string(message);
		std::cerr << "-----\n";
	}, nullptr);

	gui ui(window, context);

	std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
	//set vsync mode
	activate_vsync();
	sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);

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
	//	 x=		 y=		z=		u=	v=		normal
		-0.9f,	0.0f,	 0.9f,		0,	1, 0,1,0,
		 0.9f,	0.0f,	 0.9f,		1,	1, 0,1,0,
		-0.9f,	0.0f,	-0.9f,		0,	0, 0,1,0,
		 0.9f,	0.0f,	-0.9f,		1,	0, 0,1,0
	};

	std::vector<unsigned int> plane_indices =
	{
		0, 1, 2, //triangle 0
		1, 3, 2  //triangle 1
	};

	shader simple_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
	renderable textured_plane(simple_shader, polutropon_logo_texture,  plane, plane_indices, 
		{ true, true, true }, 3 + 2 + 3, 0, 3, 5);
	//set opengl clear color
	glClearColor(0.2f, 0.3f, 0.4f, 1);

	gltf_loader loader(simple_shader, polutropon_logo_texture);

	camera cam;
	{
		auto tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 3.f, 3.f));
		auto rot = glm::rotate(glm::mat4(1.f), glm::radians(45.f), glm::vec3(-1.f, 0, 0));
		cam.set_model(tr*rot);
	}

	camera cam_2d;
	cam_2d.set_projection_type(camera::projection_type::ortho);
	{
		
		auto tr = glm::translate(glm::mat4(1.f), glm::vec3(0, 5.f, 0));
		auto rot = glm::rotate(glm::mat4(1.f), glm::radians(90.f), glm::vec3(-1.f, 0, 0));
		cam_2d.set_model(tr*rot);
	}

	auto duck_renderable = loader.load_mesh("/gltf/Duck.glb", 0);

	scene_object plane0(textured_plane);
	scene_object plane1(textured_plane);
	scene_object plane2(textured_plane);
	scene_object duck(duck_renderable);

	//view = glm::transpose(view);
	//main loop
	glEnable(GL_DEPTH_TEST);
	while (running)
	{
		update_timing();
		const auto delta = float(current_time) - float(last_frame_time);

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


		//clear viewport
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
		const auto size = window.size();
		cam.update_projection(size.x, size.y);

		float t_in_sec =(current_time) / 1000.f;

		glm::vec3 light_position_0(5*glm::sin(t_in_sec), 3, 5*glm::cos(t_in_sec));
		std::cout << light_position_0.x << ' ' << light_position_0.z << '\n';

		duck_renderable.set_light_0_position(light_position_0);
		textured_plane.set_light_0_position(light_position_0);

		duck.set_model(glm::scale(glm::mat4(1.f), glm::vec3(0.01f)));
		//plane0.draw(cam.view_porjection_matrix());
		plane1.draw(cam);
		plane2.draw(cam);
		duck.draw(cam);

		//glClear(GL_DEPTH_BUFFER_BIT);
		//cam_2d.update_projection(size.x, size.y);
		//textured_plane.set_mvp_matrix(cam_2d.view_porjection_matrix() * glm::scale(glm::mat4(1.f), glm::vec3(0.5f)));
		//textured_plane.draw();

		draw_debug_ui();
		
		ui.render();

		//swap buffers
		window.gl_swap();
	}

	FreeImage_DeInitialise();
}
