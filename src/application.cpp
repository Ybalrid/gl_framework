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
		//if(!e.key.repeat)
		//	switch(e.key.keysym.sym)
		//	{
		//	default:break;
		//	}
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


void application::update_timing()
{
	//calculate frame timing
	last_frame_time = current_time;
	current_time = SDL_GetTicks();
	last_frame_delta = current_time - last_frame_time;

	//take care of the FPS counter
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
	//init sdl
	auto root = sdl::Root(SDL_INIT_EVERYTHING);
	sdl::Event event{};

	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLEBUFFERS, 1);
	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLESAMPLES, 16);
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); //OpenGL 4+
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 6); //OpenGL 4.6

	//create window
	auto window = sdl::Window("application window",
		{ 800, 600 }, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | SDL_WINDOW_RESIZABLE);



	//create OpenGL context
	auto context = window.create_context();
	context.make_current();
	glEnable(GL_MULTISAMPLE);



	if (glewInit() != GLEW_OK)
	{
		std::cerr << "cannot init glew\n";
		abort();
	}
	std::cout << "Initialized GLEW " << glewGetString(GLEW_VERSION) << '\n';

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

	shader unlit_shader("/shaders/simple.vert.glsl", "/shaders/unlit.frag.glsl");
	shader simple_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
	renderable textured_plane(simple_shader, polutropon_logo_texture, plane, plane_indices,
		{ true, true, true }, 3 + 2 + 3, 0, 3, 5);
	//set opengl clear color
	glClearColor(0.2f, 0.3f, 0.4f, 1);

	gltf_loader gltf(simple_shader, polutropon_logo_texture);

	camera cam;
	//cam.fov = 360;
	cam.xform.set_position({ 0,5, 5 });
	cam.xform.set_orientation(glm::angleAxis(glm::radians(-45.f), transform::X_AXIS));

	camera ortho_cam;
	ortho_cam.set_projection_type(camera::projection_type::ortho);
	ortho_cam.xform.set_position({ 0,5,0 });
	ortho_cam.xform.set_orientation(glm::angleAxis(glm::radians(-90.f), transform::X_AXIS));

	camera hud_cam;
	hud_cam.set_projection_type(camera::projection_type::hud);
	hud_cam.xform.set_position({ 0,5,0 });
	hud_cam.xform.set_orientation(glm::angleAxis(glm::radians(-90.f), transform::X_AXIS));

	auto duck_renderable = gltf.load_mesh("/gltf/Duck.glb", 0);

	renderable hud_plane(unlit_shader, polutropon_logo_texture, plane, plane_indices, { true, true, true }, 3 + 2 + 3, 0, 3, 5);
	scene_object hud(hud_plane);
	hud.xform.set_position({ 100,0,100 });
	scene_object plane0(textured_plane);
	scene_object plane1(textured_plane);
	scene_object plane2(textured_plane);
	scene_object duck(duck_renderable);
	scene_object ortho(hud_plane);
	ortho.xform.set_scale(0.25f * transform::UNIT_SCALE);
	float xortho = 0, yortho = 0, xhud = 400, yhud = 300;
	float f = 0;
	float scale_ortho = ortho.xform.get_scale().x, hud_scale = 100;

	bool draw_ortho = false, draw_hud = false;
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
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		const auto size = window.size();
		cam.update_projection(size.x, size.y);

		float t_in_sec = (current_time) / 1000.f;

		glm::vec3 light_position_0(5 * glm::sin(t_in_sec), 3, 5 * glm::cos(t_in_sec));

		duck_renderable.set_light_0_position(light_position_0);
		textured_plane.set_light_0_position(light_position_0);
		duck.xform.set_scale(0.01f * transform::UNIT_SCALE);

		duck.xform.set_orientation({ glm::radians(ceilf(float(current_time) / 1000.f)), transform::Y_AXIS });
		duck.draw(cam);

		//plane0.draw(cam.view_projection_matrix());
		plane1.xform.set_position({ 2, 0, 0 });
		plane1.draw(cam);
		plane2.draw(cam);



		ImGui::Checkbox("2D ortho pass ?", &draw_ortho);
		if (draw_ortho)
		{
			ortho.xform.set_scale(scale_ortho * transform::UNIT_SCALE);
			ortho.xform.set_position({ xortho,0,yortho });
			glClear(GL_DEPTH_BUFFER_BIT);
			ortho_cam.update_projection(size.x, size.y);
			ortho.draw(ortho_cam);
		}


		ImGui::Checkbox("2D HUD pass?", &draw_hud);
		if (draw_hud)
		{
			glClear(GL_DEPTH_BUFFER_BIT);
			hud_cam.update_projection(size.x, size.y);
			hud.xform.set_position({ xhud, f, yhud });
			hud.xform.set_scale(hud_scale * transform::UNIT_SCALE);
			hud.draw(hud_cam);
		}


		ImGui::SliderFloat("scale ortho : ", &scale_ortho, 0, 2);
		ImGui::SliderFloat("x ortho", &xortho, -1, 1);
		ImGui::SliderFloat("y ortho", &yortho, -1, 1);
		//ImGui::SliderFloat("depth", &f, -10, 10);
		ImGui::SliderFloat("scale hud", &hud_scale, 0, 500);
		ImGui::SliderFloat("x hud", &xhud, 0, float(size.x));
		ImGui::SliderFloat("y hud", &yhud, 0, float(size.y));

		draw_debug_ui();
		ui.render();

		//swap buffers
		window.gl_swap();
	}

}
