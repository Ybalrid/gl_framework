#include "application.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "renderable.hpp"
#include "camera.hpp"
#include "gui.hpp"
#include "scene_object.hpp"
#include "gltf_loader.hpp"
#include "imgui.h"
#include <cpptoml.h>

#include "light.hpp"

std::vector<std::string> application::resource_paks;
scene* application::main_scene = nullptr;

void application::activate_vsync()
{
	try
	{
		sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::adaptive_vsync);
	}
	catch(const sdl::Exception& e)
	{
		std::cerr << e.what() << '\n';
		std::cerr << "Using standard vsync instead of adaptive vsync\n";
		try
		{
			sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::vsync);
		}
		catch(const sdl::Exception& ee)
		{
			std::cerr << ee.what() << '\n';
			std::cerr << "Cannot set vsync for this driver.\n";
			try
			{
				sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);
			}
			catch(const sdl::Exception& eee)
			{
				std::cerr << eee.what() << '\n';
			}
		}
	}
}

void application::draw_debug_ui()
{
	static bool show_demo_window = false;

	if(debug_ui)
	{
		ImGui::Begin("Debug Window", &debug_ui);
		ImGui::Text("Debug information");
		ImGui::Text("FPS: %d", fps);
		ImGui::Checkbox("Show demo window ?", &show_demo_window);

		if(show_demo_window)
			ImGui::ShowDemoWindow(&show_demo_window);
		ImGui::End();
	}
}

void application::update_timing()
{
	//calculate frame timing
	last_frame_time		 = current_time;
	current_time		 = SDL_GetTicks();
	current_time_in_sec  = float(current_time) * .001f;
	last_frame_delta	 = current_time - last_frame_time;
	last_frame_delta_sec = float(last_frame_delta) * .001f;

	//take care of the FPS counter
	if(current_time - last_second_time >= 1000)
	{
		fps				 = frames_in_current_sec;
		frames_in_current_sec			 = 0;
		last_second_time = current_time;
	}
	frames_in_current_sec++;
}

void application::set_opengl_attribute_configuration(const bool multisampling, const int samples, const bool srgb_framebuffer) const
{
	int major = 4, minor = 3;
#ifdef __APPLE__
	minor = 1;
#endif

	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLEBUFFERS, multisampling);
	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLESAMPLES, samples);
	sdl::Window::gl_set_attribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, srgb_framebuffer);		 //Fragment shaders will perform individual gamma correction
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);						 //OpenGL 4+
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);						 //OpenGL 4.1 or 4.3
}

void application::initialize_glew() const
{
	if(glewInit() != GLEW_OK)
	{
		std::cerr << "cannot init glew\n";
		abort();
	}
	std::cout << "Initialized GLEW " << glewGetString(GLEW_VERSION) << '\n';
}

void application::install_opengl_debug_callback() const
{
	if(glDebugMessageCallback)
	{
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const void* user_param) {
			std::cerr << "-----\n";
			std::cerr << "opengl debug message: " << glGetString(source) << ' ' << glGetString(type) << ' ' << id << ' ' << std::string(message);
			std::cerr << "-----\n";
		},
							   nullptr);
	}
}

void application::configure_and_create_window()
{
	//load config
	auto configuration_data = resource_system::get_file("/config.toml");
	configuration_data.push_back('\0');
	const std::string configuration_text(reinterpret_cast<const char*>(configuration_data.data()));
	std::istringstream configuration_stream(configuration_text);
	auto config_toml			   = cpptoml::parser(configuration_stream);
	const auto loaded_config	   = config_toml.parse();
	const auto configuration_table = loaded_config->get_table("configuration");

	//extract config values
	const bool multisampling	= configuration_table->get_as<bool>("multisampling").value_or(true);
	const int samples			= configuration_table->get_as<int>("samples").value_or(8);
	const bool srgb_framebuffer = false; //nope, sorry. Shader will take care of gamma correction ;)

	//extract window config
	set_opengl_attribute_configuration(multisampling, samples, srgb_framebuffer);
	const bool fullscreen = configuration_table->get_as<bool>("fullscreen").value_or(false);
	sdl::Vec2i window_size;

	const auto window_size_array = configuration_table->get_array_of<int64_t>("resolution");
	window_size.x				 = int(window_size_array->at(0));
	window_size.y				 = int(window_size_array->at(1));

	//create window
	window = sdl::Window("application window",
						 window_size,
						 SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
}

void application::create_opengl_context()
{
	//create OpenGL context
	context = window.create_context();
	context.make_current();
	std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
	glEnable(GL_MULTISAMPLE);
}

scene* application::get_main_scene()
{
	return main_scene;
}

void application::initialize_modern_opengl()
{
	initialize_glew();
	install_opengl_debug_callback();
}

void application::initialize_gui()
{
	ui = gui(window.ptr(), context.ptr());
	scripts.register_imgui_library(&ui);
	ui.set_console_input_consumer(&scripts);
}

void application::render_frame()
{
	ui.frame();
	
	scripts.update(last_frame_delta_sec);

	s.scene_root->update_world_matrix();
	//The camera world matrix is stored inside the camera to permit to compute the camera view matrix
	main_camera->set_world_matrix(cam_node->get_world_matrix());
	shader_program_manager::set_frame_uniform(shader::uniform::gamma, shader::gamma);
	shader_program_manager::set_frame_uniform(shader::uniform::camera_position, cam_node->local_xform.get_position());
	shader_program_manager::set_frame_uniform(shader::uniform::view, main_camera->get_view_matrix());
	shader_program_manager::set_frame_uniform(shader::uniform::projection, main_camera->get_projection_matrix());
	shader_program_manager::set_frame_uniform(shader::uniform::main_directional_light, sun);
	shader_program_manager::set_frame_uniform(shader::uniform::point_light_0, *p_lights[0]);
	shader_program_manager::set_frame_uniform(shader::uniform::point_light_1, *p_lights[1]);
	shader_program_manager::set_frame_uniform(shader::uniform::point_light_2, *p_lights[2]);
	shader_program_manager::set_frame_uniform(shader::uniform::point_light_3, *p_lights[3]);

	glClearColor(0.4f ,0.5f,0.6f, 1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto size = window.size();
	main_camera->update_projection(size.x, size.y);

	glEnable(GL_DEPTH_TEST);
	s.run_on_whole_graph([=](node* current_node)
	{
		current_node->visit([=](auto&& node_attached_object)
		{
			using T = std::decay_t<decltype(node_attached_object)>;
			if constexpr(std::is_same_v<T, scene_object>)
			{
				//TODO instead of drawing, accumulate a buffer of thing that passes a frustum culling test
				auto& object = static_cast<scene_object&>(node_attached_object);
				object.draw(*main_camera, current_node->get_world_matrix());
			}
		});
	});

	draw_debug_ui();
	ui.render();

	//swap buffers
	window.gl_swap();

	frames++;
}

void application::run_events()
{
	//TODO move this thing to somewhere else
	//event polling
	mousex = mousey = 0;
	while(event.poll())
	{
		//For ImGui
		ui.handle_event(event);
		//Maybe move this thing too... xD
		switch(event.type)
		{
			case SDL_QUIT:
				running = false;
				break;
			case SDL_MOUSEBUTTONDOWN:
				if(ImGui::GetIO().WantCaptureMouse) break;
				mouse = true;
				break;
			case SDL_MOUSEBUTTONUP:
				if(ImGui::GetIO().WantCaptureMouse) break;
				mouse = false;
				break;
			case SDL_MOUSEMOTION:
				if(ImGui::GetIO().WantCaptureMouse) break;
				mousex = (float)event.motion.xrel;
				mousey = (float)event.motion.yrel;
				break;

			case SDL_KEYDOWN:
				if(ImGui::GetIO().WantCaptureKeyboard) break;
				if(event.key.repeat) break;
				switch(event.key.keysym.scancode)
				{
					case SDL_SCANCODE_GRAVE:
						ui.show_console = !ui.show_console;
						break;
					case SDL_SCANCODE_TAB:
						debug_ui = !debug_ui;
						break;
					case SDL_SCANCODE_W:
						up = true;
						break;
					case SDL_SCANCODE_S:
						down = true;
						break;
					case SDL_SCANCODE_A:
						left = true;
						break;
					case SDL_SCANCODE_D:
						right = true;
						break;
					default: break;
				}
				break;
			case SDL_KEYUP:
				if(ImGui::GetIO().WantCaptureKeyboard) break;
				if(event.key.repeat) break;
				switch(event.key.keysym.scancode)
				{
					case SDL_SCANCODE_W:
						up = false;
						break;
					case SDL_SCANCODE_S:
						down = false;
						break;
					case SDL_SCANCODE_A:
						left = false;
						break;
					case SDL_SCANCODE_D:
						right = false;
						break;
					default: break;
				}
				break;
			default:
				break;
		}
	}

	//TODO build real input system for this
	if(up)
		cam_node->local_xform.set_position(cam_node->local_xform.get_position()
										   + cam_node->local_xform.get_orientation()
											   * last_frame_delta_sec
											   * (glm::vec3(0, 0, -0.3f)));
	if(down)
		cam_node->local_xform.set_position(cam_node->local_xform.get_position()
										   + cam_node->local_xform.get_orientation()
											   * last_frame_delta_sec
											   * (glm::vec3(0, 0, 0.3f)));
	if(left)
		cam_node->local_xform.set_position(cam_node->local_xform.get_position()
										   + cam_node->local_xform.get_orientation()
											   * last_frame_delta_sec
											   * (glm::vec3(-0.3f, 0, 0)));
	if(right)
		cam_node->local_xform.set_position(cam_node->local_xform.get_position()
										   + cam_node->local_xform.get_orientation()
											   * last_frame_delta_sec
											   * (glm::vec3(0.3f, 0, 0)));
	if(mouse)
	{
		auto q = cam_node->local_xform.get_orientation();
		q	  = glm::rotate(q, glm::radians(mousex * last_frame_delta_sec * -5), transform::Y_AXIS);
		q	  = glm::rotate(q, glm::radians(mousey * last_frame_delta_sec * -5), transform::X_AXIS);
		cam_node->local_xform.set_orientation(q);
	}
}

void application::run()
{
	auto buffer		= audio_system::get_buffer("/sounds/rubber_duck.wav");
	auto* source	= s.scene_root->push_child(create_node())->assign(audio_source());
	ALuint alsource = source->get_al_source();

	alSourcei(alsource, AL_BUFFER, buffer.get_al_buffer());
	alSourcei(alsource, AL_LOOPING, 1);
	alSourcePlay(alsource);

	
	//TODO refactor renderloop
	while(running)
	{
		update_timing();
		run_events();

		//TOOD maybe script this thing?
		//duck_root->local_xform.set_orientation(glm::angleAxis(glm::radians((180 * current_time_in_sec)), -transform::Y_AXIS));
		//plane1->local_xform.set_position({ 2, 0, 0 });
		//plane1->local_xform.set_orientation(glm::angleAxis(glm::radians(90.f), transform::Z_AXIS));

		render_frame();
	}
}

void application::setup_scene()
{
	main_scene							   = &s;
	texture_handle polutropon_logo_texture = texture_manager::create_texture();
	{
		auto img							 = image("/polutropon.png");
		auto& polutropon_logo_texture_object = texture_manager::get_from_handle(polutropon_logo_texture);
		polutropon_logo_texture_object.load_from(img);
		polutropon_logo_texture_object.generate_mipmaps();
	}

	// clang-format off
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
	// clang-format on

	//	shader_handle unlit_shader		 = shader_program_manager::create_shader("/shaders/simple.vert.glsl", "/shaders/unlit.frag.glsl");
	shader_handle simple_shader		 = shader_program_manager::create_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
	renderable_handle textured_plane = renderable_manager::create_renderable(simple_shader, plane, plane_indices, renderable::configuration{ true, true, true }, 3 + 2 + 3, 0, 3, 5);
	renderable_manager::get_from_handle(textured_plane).set_diffuse_texture(polutropon_logo_texture);
	//set opengl clear color

	gltf = gltf_loader(simple_shader);

	cam_node = s.scene_root->push_child(create_node());
	{
		camera cam_obj;
		cam_obj.fov = 45;
		cam_node->local_xform.set_position({ 0, 5, 5 });
		cam_node->local_xform.set_orientation(glm::angleAxis(glm::radians(-45.f), transform::X_AXIS));
		cam_node->assign(std::move(cam_obj));
		main_camera = cam_node->get_if_is<camera>();
		assert(main_camera);

		cam_node->push_child(create_node())->assign(listener_marker());
	}

	auto duck_renderable = gltf.load_mesh("/gltf/Duck.glb", 0);
	auto plane0			 = s.scene_root->push_child(create_node());
	auto plane1			 = s.scene_root->push_child(create_node());
	auto duck_root		 = s.scene_root->push_child(create_node());
	auto duck			 = duck_root->push_child(create_node());
	duck->assign(scene_object(duck_renderable));
	duck->local_xform.set_scale(0.01f * transform::UNIT_SCALE);
	plane0->assign(scene_object(textured_plane));
	plane1->assign(scene_object(textured_plane));

	auto other_plane = duck_root->push_child(create_node());
	other_plane->local_xform.rotate(90, transform::X_AXIS);
	other_plane->local_xform.translate(glm::vec3(0, 2.5f, 0));
	other_plane->local_xform.scale(0.9f * transform::UNIT_SCALE);
	other_plane->assign(scene_object(textured_plane));

	sun.diffuse = sun.specular = glm::vec3(1);
	sun.specular *= 42;
	sun.ambient   = glm::vec3(0);
	sun.direction = glm::normalize(glm::vec3(-0.5f, -0.25, 1));

	std::array<node*, 4> lights{ nullptr, nullptr, nullptr, nullptr };

	lights[0] = s.scene_root->push_child(create_node());
	lights[1] = s.scene_root->push_child(create_node());
	lights[2] = s.scene_root->push_child(create_node());
	lights[3] = s.scene_root->push_child(create_node());

	for(size_t i = 0; i < 4; ++i)
	{
		auto* l		= lights[i];
		auto* pl	= l->assign(point_light());
		pl->ambient = glm::vec3(0.1f);
		pl->diffuse = pl->specular = glm::vec3(0.9f, 0.85f, 0.8f) * 1.0f / 4.0f;
		p_lights[i]				   = (pl);
	}

	lights[0]->local_xform.set_position(glm::vec3(-4.f, 3.f, -4.f));
	lights[1]->local_xform.set_position(glm::vec3(-4.f, -3.f, -4.f));
	lights[2]->local_xform.set_position(glm::vec3(-1.5f, 3.f, 1.75f));
	lights[3]->local_xform.set_position(glm::vec3(-1.f, 0.75f, 1.75f));
}

application::application(int argc, char** argv) :
 resources(argc > 0 ? argv[0] : nullptr)
{
	for(const auto& pak : resource_paks)
	{
		std::cerr << "Adding to resources " << pak << '\n';
		resource_system::add_location(pak);
	}

	configure_and_create_window();
	create_opengl_context();
	initialize_modern_opengl();
	initialize_gui();
	setup_scene();
}
