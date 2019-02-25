#pragma once
#include "application.hpp"
#include "physfs_raii.hpp"
#include "image.hpp"

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
		{ 800, 800 }, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

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

#pragma pack(push, 1)
	float plane[] = {
	//	 x=		 y=		z=		u=	v=
		-0.9,	 0.9,	0,		0,	1,
		 0.9,	 0.9,	0,		1,	1,
		-0.9,	-0.9,	0,		0,	0,
		 0.9,	-0.9,	0,		1,	0
	};

	unsigned int plane_indices[] =
	{
		0, 1, 2, //triangle 0
		1, 3, 2  //triangle 1
	};
#pragma pack(pop)

	const auto index_count = sizeof(plane_indices) / sizeof(unsigned int);

	GLuint VBO = 0, EBO = 0;
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	GLuint VAO = 0;
	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);
	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(plane), plane, GL_STATIC_DRAW);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(plane_indices), plane_indices, GL_STATIC_DRAW);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), 0);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
	glEnableVertexAttribArray(1);
	glBindVertexArray(0);

	auto frag_bytes = resource_system::get_file("/shaders/simple.frag.glsl");
	auto vert_bytes = resource_system::get_file("/shaders/simple.vert.glsl");
	//transform into C strings by pushing a null terminator
	frag_bytes.push_back(0);
	vert_bytes.push_back(0);
	const char* fragment_source = (const char*)frag_bytes.data();
	const char* vertex_source = (const char*)vert_bytes.data();

	GLint success = 0; GLchar info_log[512] = {0};
	const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
	const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(vertex_shader, 1, &vertex_source, NULL);
	glShaderSource(fragment_shader, 1,&fragment_source, NULL);
	glCompileShader(vertex_shader);
	glCompileShader(fragment_shader);
	glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(vertex_shader, sizeof(info_log), nullptr, info_log);
		std::cout << "vertex shader compile error " << info_log << '\n';
	}
	glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
	if(!success)
	{
		glGetShaderInfoLog(fragment_shader, sizeof(info_log), nullptr, info_log);
		std::cout << "fragment shader compile error " << info_log << '\n';
	}
	GLuint program = glCreateProgram();
	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if(!success)
	{
		glGetProgramInfoLog(program, sizeof(info_log), nullptr, info_log);
		std::cout << "shader program link error " << info_log << '\n';
	}
	glDeleteShader(vertex_shader);
	glDeleteShader(fragment_shader);

	//set opengl clear color
	glClearColor(0.2f, 0.3f, 0.4f, 1);

	//main loop
	while (running)
	{
		//event polling
		while (event.poll())
		{
			switch (event.type)
			{
			case SDL_QUIT:
				running = false;
			default:
				handle_event(event);
				break;
			}
		}

		//clear viewport
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(program);
		polutropon_logo_texture.bind();
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);

		//swap buffers
		window.gl_swap();
	}

	FreeImage_DeInitialise();
}
