#pragma once


#include <GL/glew.h>

#include <vector>
#include <string>

namespace sdl
{
	struct Window;
	union Event;
}

struct SDL_Window;
typedef void* SDL_GLContext;
struct console_input_consumer
{
	virtual ~console_input_consumer() = default;
	virtual bool operator()(const std::string& str) = 0;
};

class gui
{
	SDL_Window* w = nullptr;

	void console();
	char console_input[256]={0};
	std::vector<std::string> console_content{"Debuging console."};
	bool scroll_console_to_bottom = false;
	console_input_consumer* cis_ptr = nullptr;
public:

	bool show_console = true;
	gui() = default;
	gui(SDL_Window* window, SDL_GLContext gl_context);
	~gui();
	void frame();
	void render();
	void handle_event(sdl::Event e);

	void set_console_input_consumer(console_input_consumer* cis);
	void push_to_console(const std::string& str);

	gui(const gui&) = delete;
	gui& operator=(const gui&) = delete;
	gui(gui&& o) noexcept
	{
		w = o.w;
		o.w = nullptr;
	}

	gui& operator=(gui&& o) noexcept
	{
		w = o.w;
		o.w = nullptr;
		return *this;
	}
};

