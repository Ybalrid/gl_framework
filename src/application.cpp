#include "application.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "renderable.hpp"
#include "camera.hpp"
#include "gui.hpp"
#include "scene_object.hpp"
#include "gltf_loader.hpp"
#include <cpptoml.h>

#include "light.hpp"

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
		catch (const sdl::Exception& ee)
		{
			std::cerr << ee.what() << '\n';
			std::cerr << "Cannot set vsync for this driver.\n";
			try
			{
				sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);
			}
			catch (const sdl::Exception& eee)
			{
				std::cerr << eee.what() << '\n';
			}
		}
	}
}

void application::handle_event(const sdl::Event& e)
{
	switch (e.type)
	{
	case SDL_KEYDOWN:
		if (ImGui::GetIO().WantCaptureKeyboard) break;
		if (!e.key.repeat)
			switch (e.key.keysym.sym)
			{
			case SDLK_TAB:
				debug_ui = !debug_ui;
				break;
			default:break;
			}
		break;
	case SDL_KEYUP:
		if (ImGui::GetIO().WantCaptureKeyboard) break;
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
	static bool show_demo_window = false;


	if (debug_ui)
	{
		ImGui::Begin("Debug Window", &debug_ui);
		ImGui::Text("Debug information");
		ImGui::Text("FPS: %d", fps);
		ImGui::Checkbox("Show demo window ?", &show_demo_window);

		if (show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		ImGui::End();
	}
}


void application::update_timing()
{
	//calculate frame timing
	last_frame_time = current_time;
	current_time = SDL_GetTicks();
	current_time_in_sec = float(current_time) *.001f;
	last_frame_delta = current_time - last_frame_time;
	last_frame_delta_sec = float(last_frame_delta) *.001f;

	//take care of the FPS counter
	if (current_time - last_second_time >= 1000)
	{
		fps = frames;
		frames = 0;
		last_second_time = current_time;
	}
	frames++;
}

void application::set_opengl_attribute_configuration(const bool multisampling, const int samples, const bool srgb_framebuffer) const
{
	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLEBUFFERS, multisampling);
	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLESAMPLES, samples);
	sdl::Window::gl_set_attribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, srgb_framebuffer); //Fragment shaders will perform individual gamma correction
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4); //OpenGL 4+
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, 6); //OpenGL 4.6
}

void application::initialize_glew() const
{
	if (glewInit() != GLEW_OK)
	{
		std::cerr << "cannot init glew\n";
		abort();
	}
	std::cout << "Initialized GLEW " << glewGetString(GLEW_VERSION) << '\n';
}

void application::install_opengl_debug_callback() const
{
	glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length,
		const GLchar* message, const void* user_param)
	{
		std::cerr << "-----\n";
		std::cerr << "opengl debug message: " << glGetString(source) << ' ' << glGetString(type) << ' ' << id << ' ' <<
			std::string(message);
		std::cerr << "-----\n";
	}, nullptr);
}

void application::configure_and_create_window()
{
	//load config
	auto configuration_data = resource_system::get_file("/config.toml");
	configuration_data.push_back('\0');
	const std::string configuration_text(reinterpret_cast<const char*>(configuration_data.data()));
	std::istringstream configuration_stream(configuration_text);
	auto config_toml = cpptoml::parser(configuration_stream);
	const auto loaded_config = config_toml.parse();
	const auto configuration_table = loaded_config->get_table("configuration");

	//extract config values
	const bool multisampling = configuration_table->get_as<bool>("multisampling").value_or(true);
	const int samples = configuration_table->get_as<int>("samples").value_or(8);
	const bool srgb_framebuffer = false; //nope, sorry. Shader will take care of gamma correction ;)

	//extract window config
	set_opengl_attribute_configuration(multisampling, samples, srgb_framebuffer);
	const bool fullscreen = configuration_table->get_as<bool>("fullscreen").value_or(false);
	sdl::Vec2i window_size{};

	const auto window_size_array = configuration_table->get_array_of<int64_t>("resolution");
	window_size.x = int(window_size_array->at(0));
	window_size.y = int(window_size_array->at(1));

	//create window
	window = sdl::Window("application window",
		window_size, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
}

void application::create_opengl_context()
{
	//create OpenGL context
	context = window.create_context();
	context.make_current();
	std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
	glEnable(GL_MULTISAMPLE);
}

application::application(int argc, char** argv) : resources(argc > 0 ? argv[0] : nullptr)
{
	for (const auto& pak : resource_paks)
	{
		std::cerr << "Adding to resources " << pak << '\n';
		resource_system::add_location(pak);
	}

	configure_and_create_window();

	create_opengl_context();

	initialize_glew();
	install_opengl_debug_callback();
	ui = gui(window, context);
	scripts.register_imgui_library();

	//set vsync mode
	activate_vsync();

	texture polutropon_logo_texture;
	{
		auto img = image("/polutropon.png");
		polutropon_logo_texture.load_from(img);
		polutropon_logo_texture.generate_mipmaps();
	}

	std::vector<float> plane =
	{
		//x=	y=		 z=			u=	v=		normal=
		-0.9f,	0.0f,	 0.9f,		0,	1,		0, 1, 0,
		 0.9f,	0.0f,	 0.9f,		1,	1,		0, 1, 0,
		-0.9f,	0.0f,	-0.9f,		0,	0,		0, 1, 0,
		 0.9f,	0.0f,	-0.9f,		1,	0,		0, 1, 0,
	};

	std::vector<unsigned int> plane_indices =
	{
		0, 1, 2, //triangle 0
		1, 3, 2  //triangle 1
	};

	shader unlit_shader("/shaders/simple.vert.glsl", "/shaders/unlit.frag.glsl");
	shader simple_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
	renderable textured_plane(&simple_shader, plane, plane_indices,
		{ true, true, true }, 3 + 2 + 3, 0, 3, 5);
	textured_plane.set_diffuse_texture(&polutropon_logo_texture);
	//set opengl clear color

	gltf = gltf_loader(simple_shader);

	camera cam;
	cam.fov = 45;
	cam.xform.set_position({ 0,5, 5 });
	cam.xform.set_orientation(glm::angleAxis(glm::radians(-45.f), transform::X_AXIS));

	auto duck_renderable = gltf.load_mesh("/gltf/Duck.glb", 0);

	scene_object plane1(textured_plane);
	scene_object plane2(textured_plane);
	scene_object duck(duck_renderable);

	directional_light sun;
	sun.diffuse = sun.specular = glm::vec3(1);
	sun.specular *= 42;
	sun.ambient = glm::vec3(0);
	sun.direction = glm::normalize(glm::vec3(-0.5f, -0.25, 1));

	point_light lights[4];
	lights[0].position = glm::vec3(-4.f, 3.f, -4.f);
	lights[1].position = glm::vec3(-4.f, -3.f, -4.f);
	lights[2].position = glm::vec3(-1.5f, 3.f, 1.75f);

	lights[0].diffuse = lights[0].ambient = lights[0].specular = glm::vec3(1.f, 0, 0) *0.025f;
	lights[1].diffuse = lights[1].ambient = lights[1].specular = glm::vec3(0, 1.f, 0) *0.05f;
	lights[2].diffuse = lights[2].ambient = lights[2].specular = glm::vec3(0, 0, 1.f) *0.4f;
	lights[3].diffuse = lights[3].ambient = lights[3].specular = glm::vec3(1.f);
	lights[3].position = glm::vec3(-0.f, 4.f, 3.5f);

	glEnable(GL_DEPTH_TEST);

	std::vector<float> cube_vertex =
	{
		-.5f, -.5f,  .5f,	0, 0,	 0, 0, 1,
		 .5f, -.5f,  .5f,	1, 0,	 0, 0, 1,
		 .5f,  .5f,  .5f,	1, 1,	 0, 0, 1,
		-.5f,  .5f,  .5f,	0, 1,	 0, 0, 1,

		-.5f,  .5f, -.5f,	0, 0,	 0, 1, 0,
		 .5f,  .5f, -.5f,	1, 0,	 0, 1, 0,
		 .5f,  .5f,  .5f,	1, 1,	 0, 1, 0,
		-.5f,  .5f,  .5f,	0, 1,	 0, 1, 0,

		-.5f, -.5f, -.5f,	0, 0,	 0,-1, 0,
		 .5f, -.5f, -.5f,	1, 0,	 0,-1, 0,
		 .5f, -.5f,  .5f,	1, 1,	 0,-1, 0,
		-.5f, -.5f,  .5f,	0, 1,	 0,-1, 0,

		-.5f, -.5f, -.5f,	0, 0,	 0, 0,-1,
		 .5f, -.5f, -.5f,	1, 0,	 0, 0,-1,
		 .5f,  .5f, -.5f,	1, 1,	 0, 0,-1,
		-.5f,  .5f, -.5f,	0, 1,	 0, 0,-1,

		-.5f,  .5f, -.5f,	0, 0,	-1, 0, 0,
		-.5f,  .5f,  .5f,	1, 0,	-1, 0, 0,
		-.5f, -.5f,  .5f,	1, 1,	-1, 0, 0,
		-.5f, -.5f, -.5f,	0, 1,	-1, 0, 0,

		 .5f,  .5f, -.5f,	0, 0,	 1, 0, 0,
		 .5f,  .5f,  .5f,	1, 0,	 1, 0, 0,
		 .5f, -.5f,  .5f,	1, 1,	 1, 0, 0,
		 .5f, -.5f, -.5f,	0, 1,	 1, 0, 0,
	};

	std::vector<unsigned int> cube_index =
	{
		0,  1,  2,  2,  3,  0,
		4,  5,  6,  6,  7,  4,
		8,  9,  10, 10, 11, 8,
		12, 13, 14, 14, 15, 12,
		16, 17, 18, 18, 19, 16,
		20, 21, 22, 22, 23, 20,
	};


	renderable cube_renderable(&simple_shader, 
		cube_vertex, 
		cube_index, 
		{ true, 
			true, 
			true }, 
		3 + 3 + 2, 
		0, 
		3, 
		5);

	texture cube_diffuse, cube_specular;
	{
		image cube_diffuse_image("/textures/container2.png");
		image cube_specular_image("/textures/container2_specular.png");
		cube_diffuse.load_from(cube_diffuse_image);
		cube_specular.load_from(cube_specular_image);
		cube_diffuse.generate_mipmaps();
		cube_specular.generate_mipmaps();
	}

	cube_renderable.set_diffuse_texture(&cube_diffuse);
	cube_renderable.set_specular_texture(&cube_specular);

	scene_object cube0(cube_renderable);
	cube0.xform.set_position({ -1.5f, 1.f, 2.f });
	cube_renderable.mat.shininess = 128;

	bool up = false, down = false, left = false, right = false, mouse = false;
	float mousex = 0, mousey = 0;

	//TODO refactor renderloop
	while (running)
	{
		update_timing();

		//TODO move this thing to somewhere else
		//event polling
		mousex = mousey = 0;
		while (event.poll())
		{
			//For ImGui
			ui.handle_event(event);
			//For us
			handle_event(event);

			//Maybe move this thing too... xD
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
			case SDL_MOUSEBUTTONDOWN:
				if (ImGui::GetIO().WantCaptureMouse) break;
				mouse = true;
				break;
			case SDL_MOUSEBUTTONUP:
				if (ImGui::GetIO().WantCaptureMouse) break;
				mouse = false;
				break;
			case SDL_MOUSEMOTION:
				if (ImGui::GetIO().WantCaptureMouse) break;
				mousex = (float)event.motion.xrel;
				mousey = (float)event.motion.yrel;
				break;

			case SDL_KEYDOWN:
				if (ImGui::GetIO().WantCaptureKeyboard) break;
				if (event.key.repeat) break;
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
				case SDLK_z:
					up = true;
					break;
				case SDLK_s:
					down = true;
					break;
				case SDLK_q:
				case SDLK_a:
					left = true;
					break;
				case SDLK_d:
					right = true;
					break;
				default:break;
				}
				break;
			case SDL_KEYUP:
				if (ImGui::GetIO().WantCaptureKeyboard) break;
				if (event.key.repeat) break;
				switch (event.key.keysym.sym)
				{
				case SDLK_w:
				case SDLK_z:
					up = false;
					break;
				case SDLK_s:
					down = false;
					break;
				case SDLK_q:
				case SDLK_a:
					left = false;
					break;
				case SDLK_d:
					right = false;
					break;
				default:break;
				}
				break;
			default:
				break;
			}
		}

		//TODO build real input system for this
		if (up)
			cam.xform.set_position(cam.xform.get_position() + cam.xform.get_orientation() * last_frame_delta_sec * (glm::vec3(0, 0, -3)));
		if (down)
			cam.xform.set_position(cam.xform.get_position() + cam.xform.get_orientation() * last_frame_delta_sec * (glm::vec3(0, 0, 3)));
		if (left)
			cam.xform.set_position(cam.xform.get_position() + cam.xform.get_orientation() * last_frame_delta_sec * (glm::vec3(-3, 0, 0)));
		if (right)
			cam.xform.set_position(cam.xform.get_position() + cam.xform.get_orientation() * last_frame_delta_sec * (glm::vec3(3, 0, 0)));
		if (mouse)
		{
			auto q = cam.xform.get_orientation();
			q = glm::rotate(q, glm::radians(mousex * last_frame_delta_sec * -5), transform::Y_AXIS);
			q = glm::rotate(q, glm::radians(mousey * last_frame_delta_sec * -5), transform::X_AXIS);
			cam.xform.set_orientation(q);
		}

		ui.frame();
		scripts.update(last_frame_delta_sec);

		//TOOD add this to 
		duck.xform.set_scale(0.01f * transform::UNIT_SCALE);
		duck.xform.set_orientation(glm::angleAxis(glm::radians((180 * current_time_in_sec)), -transform::Y_AXIS));
		plane1.xform.set_position({ 2, 0, 0 });
		plane1.xform.set_orientation(glm::angleAxis(glm::radians(90.f), transform::Z_AXIS));
		cube0.xform.set_orientation(duck.xform.get_orientation());

		shader::set_frame_uniform(shader::uniform::gamma, shader::gamma);
		shader::set_frame_uniform(shader::uniform::camera_position, cam.xform.get_position());
		shader::set_frame_uniform(shader::uniform::view, cam.get_view_matrix());
		shader::set_frame_uniform(shader::uniform::projection, cam.get_projection_matrix());
		shader::set_frame_uniform(shader::uniform::main_directional_light, sun);
		shader::set_frame_uniform(shader::uniform::point_light_0, lights[0]);
		shader::set_frame_uniform(shader::uniform::point_light_1, lights[1]);
		shader::set_frame_uniform(shader::uniform::point_light_2, lights[2]);
		shader::set_frame_uniform(shader::uniform::point_light_3, lights[3]);

		glClearColor(0.1, 0.3, 0.5, 1);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const auto size = window.size();
		cam.update_projection(size.x, size.y);
		duck.draw(cam);
		plane1.draw(cam);
		plane2.draw(cam);
		cube0.draw(cam);

		draw_debug_ui();
		ui.render();
		
		//swap buffers
		window.gl_swap();
	}
}
