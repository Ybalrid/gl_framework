#pragma once
#include "application.hpp"
#include "physfs_raii.hpp"

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

	//in at least one of these resource pack, there's the polutropon logo at it's root
	std::vector<uint8_t> data = resource_system::get_file("/polutropon.png");

	FIBITMAP* image = nullptr;
	if (!data.empty())
	{
		const auto image_memory_stream = FreeImage_OpenMemory(data.data(), data.size());
		const auto type = FreeImage_GetFileTypeFromMemory(image_memory_stream);
		if (type != FIF_UNKNOWN)
		{
			image = FreeImage_LoadFromMemory(type, image_memory_stream);
		}
		FreeImage_CloseMemory(image_memory_stream);
	}
	//drop the orignially read data
	data.clear();

	GLuint polutropon_logo_texture = 0;
	GLint color_type = GL_RGBA;
	//image has been loaded - put it in an opengl texture inside the GPU
	if(image)
	{
		const auto w = FreeImage_GetWidth(image);
		const auto h = FreeImage_GetHeight(image);
		const auto px_count = w * h;
		const auto bpp = FreeImage_GetBPP(image);
		const auto bits = FreeImage_GetBits(image);
		const auto red_mask = FreeImage_GetRedMask(image);
		const auto blue_mask = FreeImage_GetBlueMask(image);

		if (bpp == 32 && red_mask > blue_mask)
		{
			auto* data_array = reinterpret_cast<uint32_t*>(bits);
			for(size_t i = 0; i < px_count; ++i)
			{
				const auto pixel = data_array[i];
				const auto a = 0xff000000 & pixel;
				const auto r = 0x00ff0000 & pixel;
				const auto g = 0x0000ff00 & pixel;
				const auto b = 0x000000ff & pixel;

				data_array[i] = (a) | (b << 16) | (g) | (r >> 16 );
			}
		}

		//no alpha in 24bits
		if(bpp == 24)
		{
			color_type = GL_RGB;
			if (red_mask > blue_mask)
			{
				auto* data_array = reinterpret_cast<uint8_t*>(bits);
				for (size_t i = 0; i < px_count; ++i)
				{
					std::swap(data_array[i * 3 + 0], data_array[i * 3 + 2]);
				}
			}
		}

		//Create OpenGL texture
		glGenTextures(1, &polutropon_logo_texture);
		glBindTexture(GL_TEXTURE_2D, polutropon_logo_texture);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, color_type, w, h, 0, color_type, GL_UNSIGNED_BYTE, bits);
		glGenerateMipmap(GL_TEXTURE_2D);

		FreeImage_Unload(image);
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

	const char* vertex_source = R"GLSL(

#version 330 core

layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

out vec2 texture_coords;


void main()
{
	gl_Position = vec4(in_pos, 1.0);
	texture_coords = in_uv;
}

)GLSL";

	const char* fragment_source = R"GLSL(

#version 330 core

out vec4 FragColor;
in vec2 texture_coords;
uniform sampler2D in_texture;

void main()
{
	FragColor = texture(in_texture, texture_coords);
}

)GLSL";

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
		glBindTexture(GL_TEXTURE_2D, polutropon_logo_texture);
		glBindVertexArray(VAO);
		glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_INT, nullptr);

		//swap buffers
		window.gl_swap();
	}

	FreeImage_DeInitialise();
}
