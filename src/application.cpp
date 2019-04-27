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

//The statics
std::vector<std::string> application::resource_paks;
scene* application::main_scene = nullptr;

glm::vec4 application::clear_color { 0.4f, 0.5f, 0.6f, 1.f };

GLuint bbox_drawer_vbo, bbox_drawer_vao;
shader_handle white_debug_shader = shader_program_manager::invalid_shader;

void application::activate_vsync() const
{
	try
	{
		sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::adaptive_vsync);
	}
	catch(const sdl::Exception& cannot_adaptive)
	{
		std::cerr << cannot_adaptive.what() << '\n';
		std::cerr << "Using standard vsync instead of adaptive vsync\n";
		try
		{
			sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::vsync);
		}
		catch(const sdl::Exception& cannot_standard_vsync)
		{
			std::cerr << cannot_standard_vsync.what() << '\n';
			std::cerr << "Cannot set vsync for this driver.\n";
			try
			{
				sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);
			}
			catch(const sdl::Exception& cannot_vsync_at_all)
			{
				std::cerr << cannot_vsync_at_all.what() << '\n';
			}
		}
	}
}

void application::draw_debug_ui()
{
	static auto show_demo_window  = false;
	static auto show_style_editor = false;

	if(debug_ui)
	{
		sdl::Mouse::set_relative(false);
		if(ImGui::Begin("Debugger Window", &debug_ui))
		{
			ImGui::Text("FPS: %d", fps);
			ImGui::Checkbox("Show ImGui demo window ?", &show_demo_window);
			ImGui::Checkbox("Show ImGui style editor ?", &show_style_editor);

			if(show_demo_window)
				ImGui::ShowDemoWindow(&show_demo_window);
		}
		ImGui::End();

		if(show_style_editor)
		{
			if(ImGui::Begin("Style Editor", &show_style_editor))
				ImGui::ShowStyleEditor();
			ImGui::End();
		}
	}

#ifndef NON_NAGGING_DEBUG
#ifdef _DEBUG
	const int x = 400, y = 75;
	ImGui::SetNextWindowPos({ 0, float(window.size().y - y) });
	ImGui::SetNextWindowSize({ float(x), float(y) });
	if(ImGui::Begin("Development build", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
	{
		ImGui::Text("This program is a development Debug Build.");
#if defined(USING_JETLIVE)
		ImGui::Text("Dynamic recompilation is available via jet-live.");
		ImGui::Text("Change a source file, and hit Ctrl+R to hotload.");
#elif defined(WIN32)
		ImGui::Text("Dynamic hot-reload of code is available via blink.");
		ImGui::Text("Attach blink to pid %d to hot-reload changed code.", _getpid());
#endif //dynamic hotreloader
	}
	ImGui::End();
#endif //_DEBUG
#endif //NON_NAGGING_DEBUG
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
		fps					  = frames_in_current_sec;
		frames_in_current_sec = 0;
		last_second_time	  = current_time;
	}
	frames_in_current_sec++;
}

void application::set_opengl_attribute_configuration(const bool multisampling, const int samples, const bool srgb_framebuffer) const
{
	const auto major = 4;
#ifndef __APPLE__
	const auto minor = 3;
#else
	const auto minor = 1; //Apple actively </3 OpenGL :'-(
#endif

	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLEBUFFERS, multisampling);
	sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLESAMPLES, samples);
	sdl::Window::gl_set_attribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE, srgb_framebuffer);		 //Fragment shaders will perform individual gamma correction
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);						 //OpenGL 4+
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);						 //OpenGL 4.1 or 4.3
#ifdef _DEBUG
	sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG); //create a so-called "debug" context
#endif
}

void application::initialize_glew() const
{
	if(const auto state = glewInit(); state != GLEW_OK)
	{
		std::cerr << "cannot init glew\n";
		std::cerr << glewGetErrorString(state);
		abort();
	}
	std::cout << "Initialized GLEW " << glewGetString(GLEW_VERSION) << '\n';
}

void application::install_opengl_debug_callback() const
{
	if(glDebugMessageCallback)
	{
		glEnable(GL_DEBUG_OUTPUT);
		glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
		glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei /*length*/, const GLchar* message, const void* /*user_param*/) {
			if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
			std::cerr << "-----\n";
			std::cerr << "opengl debug message: source(" << (source) << ") type(" << (type) << ") id(" << id << ") message(" << std::string(message) << ")\n";
			std::cerr << "-----" << std::endl; //flush here
		},
							   nullptr);
	}
}

void application::configure_and_create_window(const std::string& application_name)
{
	//load config
	auto configuration_data = resource_system::get_file("/config.toml");
	configuration_data.push_back('\0'); //add a string terminator
	const std::string configuration_text(reinterpret_cast<const char*>(configuration_data.data()));
	std::istringstream configuration_stream(configuration_text);
	auto config_toml			   = cpptoml::parser(configuration_stream);
	const auto loaded_config	   = config_toml.parse();
	const auto configuration_table = loaded_config->get_table("configuration");

	//extract config values
	const auto multisampling	= configuration_table->get_as<bool>("multisampling").value_or(true);
	const auto samples			= configuration_table->get_as<int>("samples").value_or(8);
	const auto srgb_framebuffer = false; //nope, sorry. Shader will take care of gamma correction ;)

	//extract window config
	set_opengl_attribute_configuration(multisampling, samples, srgb_framebuffer);
	const auto fullscreen = configuration_table->get_as<bool>("fullscreen").value_or(false);
	sdl::Vec2i window_size;

	const auto window_size_array = configuration_table->get_array_of<int64_t>("resolution");
	window_size.x				 = int(window_size_array->at(0));
	window_size.y				 = int(window_size_array->at(1));

	//create window
	window = sdl::Window(application_name,
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

void application::initialize_modern_opengl() const
{
	initialize_glew();
	install_opengl_debug_callback();
}

void application::initialize_gui()
{
	ui = gui(window.ptr(), context.ptr());
	scripts.register_imgui_library(&ui);
	ui.set_script_engine_ptr(&scripts);
	ui.set_console_input_consumer(&scripts);
	inputs.setup_imgui();
}

void application::frame_prepare()
{
	const auto opengl_debug_tag = opengl_debug_group("application::frame_prepare()");

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

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	const auto size = window.size();
	main_camera->update_projection(size.x, size.y);
}

void application::draw_full_scene_from_main_camera()
{
	const auto opengl_debug_tag = opengl_debug_group("application::draw_full_scene_from_main_camera()");

	glEnable(GL_DEPTH_TEST);
	s.run_on_whole_graph([=](node* current_node) {
		current_node->visit([=](auto&& node_attached_object) {
			using T = std::decay_t<decltype(node_attached_object)>;
			if constexpr(std::is_same_v<T, scene_object>)
			{
				//TODO instead of drawing, accumulate a buffer of thing that passes a frustum culling test
				auto& object			   = static_cast<scene_object&>(node_attached_object);
				const auto oriented_points = object.get_obb(current_node->get_world_matrix());

				//for(const auto& point : oriented_points)
				//	std::cout << "bounding box " << point.x << ' ' << point.y << ' ' << point.z << '\n';

				glBindVertexArray(bbox_drawer_vao);
				glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(float), oriented_points.data(), GL_STREAM_DRAW);
				const auto& debug_shader_object = shader_manager.get_from_handle(white_debug_shader);
				debug_shader_object.use();
				debug_shader_object.set_uniform(shader::uniform::view, main_camera->get_view_matrix());
				debug_shader_object.set_uniform(shader::uniform::projection, main_camera->get_projection_matrix());
				glPointSize(5);
				glLineWidth(5);
				const unsigned short int lines[] {0,1,1,2,2,3,3,0,4,5,5,6,6,7,7,4,0,4,1,5,2,6,3,7};
				glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, &lines);
				glBindVertexArray(0);

				object.draw(*main_camera, current_node->get_world_matrix());
			}
		});
	});
}

void application::render_frame()
{
	ui.frame();
	scripts.update(last_frame_delta_sec);

	frame_prepare();
	draw_full_scene_from_main_camera();
	draw_debug_ui();
	ui.render();

	//swap buffers
	window.gl_swap();

	frames++;
}

void application::run_events()
{
	//event polling
	while(event.poll())
	{
		//Check windowing events
		if(event.type == SDL_QUIT)
			running = false;

		//For ImGui
		ui.handle_event(event);

		//Process input events
		if(const auto command { inputs.process_input_event(event) }; command)
			command->execute();
	}

	fps_camera_controller->apply_movement(last_frame_delta_sec);
}

void application::run()
{
	while(running)
	{
#ifdef _DEBUG
#ifdef USING_JETLIVE
		liveInstance.update();
#endif
#endif
		update_timing();
		run_events();
		render_frame();
	}
}

void application::setup_scene()
{
	//TODO everything about this sould be done depending on some input somewhere, or provided by the game code - also, a level system would be useful
	main_scene									 = &s;
	const texture_handle polutropon_logo_texture = texture_manager::create_texture();
	{
		const auto img						 = image("/polutropon.png");
		auto& polutropon_logo_texture_object = texture_manager::get_from_handle(polutropon_logo_texture);
		polutropon_logo_texture_object.load_from(img);
		polutropon_logo_texture_object.generate_mipmaps();
		polutropon_logo_texture_object.set_filtering_parameters();
	}

	// clang-format off
	const std::vector<float> plane =
	{
		//x=	y=		 z=			u=	v=		normal=
		-0.9f,	0.0f,	 0.9f,		0,	1,		0, 1, 0,
		0.9f,	0.0f,	 0.9f,		1,	1,		0, 1, 0,
		-0.9f,	0.0f,	-0.9f,		0,	0,		0, 1, 0,
		0.9f,	0.0f,	-0.9f,		1,	0,		0, 1, 0,
	};

	const std::vector<unsigned int> plane_indices =
	{
		0, 1, 2, //triangle 0
		1, 3, 2  //triangle 1
	};
	// clang-format on

	const shader_handle simple_shader	  = shader_program_manager::create_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
	const renderable_handle textured_plane = renderable_manager::create_renderable(simple_shader, plane, plane_indices, renderable::aabb { { -0.9f, 0.f, -0.9f }, { 0.9f, 0.f, 0.9f } }, renderable::configuration { true, true, true }, 3 + 2 + 3, 0, 3, 5);
	renderable_manager::get_from_handle(textured_plane).set_diffuse_texture(polutropon_logo_texture);

	gltf = gltf_loader(simple_shader);

	cam_node = s.scene_root->push_child(create_node());
	{
		camera cam_obj;
		cam_obj.fov = 45;
		cam_node->local_xform.set_position({ 0, 1.75f, 5 });
		cam_node->local_xform.set_orientation(glm::angleAxis(glm::radians(-45.f), transform::X_AXIS));
		cam_node->assign(std::move(cam_obj));
		main_camera = cam_node->get_if_is<camera>();
		assert(main_camera);

		cam_node->push_child(create_node())->assign(listener_marker());
	}
	fps_camera_controller = std::make_unique<camera_controller>(cam_node);

	inputs.register_keypress(SDL_SCANCODE_A, fps_camera_controller->press(camera_controller_command::movement_type::left));
	inputs.register_keypress(SDL_SCANCODE_D, fps_camera_controller->press(camera_controller_command::movement_type::right));
	inputs.register_keypress(SDL_SCANCODE_W, fps_camera_controller->press(camera_controller_command::movement_type::up));
	inputs.register_keypress(SDL_SCANCODE_S, fps_camera_controller->press(camera_controller_command::movement_type::down));
	inputs.register_keyrelease(SDL_SCANCODE_A, fps_camera_controller->release(camera_controller_command::movement_type::left));
	inputs.register_keyrelease(SDL_SCANCODE_D, fps_camera_controller->release(camera_controller_command::movement_type::right));
	inputs.register_keyrelease(SDL_SCANCODE_W, fps_camera_controller->release(camera_controller_command::movement_type::up));
	inputs.register_keyrelease(SDL_SCANCODE_S, fps_camera_controller->release(camera_controller_command::movement_type::down));
	inputs.register_mouse_motion_command(fps_camera_controller->mouse_motion());
	inputs.register_keyany(SDL_SCANCODE_LSHIFT, fps_camera_controller->run());

	//TODO build a real level system!
	const auto duck_renderable = gltf.load_mesh("/gltf/Duck.glb", 0);
	auto plane0				   = s.scene_root->push_child(create_node());
	auto plane1				   = s.scene_root->push_child(create_node());
	auto duck_root			   = s.scene_root->push_child(create_node());
	auto duck				   = duck_root->push_child(create_node());
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

	std::array<node*, 4> lights { nullptr, nullptr, nullptr, nullptr };

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

	glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
}

void application::set_clear_color(glm::vec4 color)
{
	if(clear_color != color)
	{
		clear_color = color;
		glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
	}
}

application::application(int argc, char** argv, const std::string& application_name) :
 resources(argc > 0 ? argv[0] : nullptr)
{
	inputs.register_keypress(SDL_SCANCODE_GRAVE, &keyboard_debug_utilities.toggle_console_keyboard_command);
	inputs.register_keypress(SDL_SCANCODE_TAB, &keyboard_debug_utilities.toggle_debug_keyboard_command);
	inputs.register_keyrelease(SDL_SCANCODE_R, &keyboard_debug_utilities.toggle_live_code_reload_command);

	for(const auto& pak : resource_paks)
	{
		std::cerr << "Adding to resources " << pak << '\n';
		resource_system::add_location(pak);
	}

	//const auto files = resource_system::list_files("/", true);
	//for(const auto& file : files)
	//{
	//	std::cout << file << '\n';
	//}

	configure_and_create_window(application_name);
	create_opengl_context();
	initialize_modern_opengl();
	initialize_gui();
	setup_scene();

	sdl::Mouse::set_relative(true);

	//shader that draws everything in white
	try
	{
		white_debug_shader = shader_manager.create_shader("/shaders/debug.vert.glsl", "/shaders/debug.frag.glsl");
		glGenBuffers(1, &bbox_drawer_vbo);
		glGenVertexArrays(1, &bbox_drawer_vao);
		glBindVertexArray(bbox_drawer_vao);
		glBindBuffer(GL_ARRAY_BUFFER, bbox_drawer_vbo);
		float empty[8 * 3] {0};
		glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(float), empty, GL_STREAM_DRAW);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)(0));
		glEnableVertexAttribArray(0);
		glBindVertexArray(0);
		
	}
	catch(const std::exception& e)
	{
		sdl::show_message_box(SDL_MESSAGEBOX_WARNING, "No debug shader", e.what());
	}

}

void application::keyboard_debug_utilities_::toggle_console_keyboard_command_::execute()
{
	if(parent_->ui.is_console_showed())
	{
		parent_->ui.hide_console();
		sdl::Mouse::set_relative(true);
	}
	else
	{
		parent_->ui.show_console();
		sdl::Mouse::set_relative(false);
	}
}

void application::keyboard_debug_utilities_::toggle_debug_keyboard_command_::execute()
{
	parent_->debug_ui = !parent_->debug_ui;
}

void application::keyboard_debug_utilities_::toggle_live_code_reload_command_::execute()
{
#ifdef _DEBUG
	if(modifier & KMOD_LCTRL)
	{
#ifdef USING_JETLIVE
		parent_->liveInstance.tryReload();
#else
		static bool first_print = false;
		if(!first_print)
			parent_->ui.push_to_console("If you are using blink, live reload is automatic.");
		first_print = true;
#endif
	}
#endif
}
