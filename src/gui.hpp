#pragma once

#include <GL/glew.h>

#include <vector>
#include <string>

namespace sdl
{
	class Window;
	union Event;
}

struct SDL_Window;
typedef void* SDL_GLContext;
struct console_input_consumer
{
	virtual ~console_input_consumer()				= default;
	virtual bool operator()(const std::string& str) = 0;
};

struct ImFont;
class script_system;
class gui
{
	SDL_Window* w = nullptr;

	void console();
	char console_input[2048] = { 0 };
	std::vector<std::string> console_content {};
	std::vector<std::string> console_history {};
	bool scroll_console_to_bottom   = false;
	console_input_consumer* cis_ptr = nullptr;
	int history_counter				= 0;

	ImFont* console_font;
	ImFont* default_font;
	ImFont* ugly_font;

	void move_from(gui& other);

	script_system* scripting_engine;

	bool show_console_	 = false;
	bool last_frame_showed = false;

public:
	gui() = default;
	gui(SDL_Window* window, SDL_GLContext gl_context);
	~gui();

	void frame();
	void render() const;
	void handle_event(sdl::Event e) const;

	void set_console_input_consumer(console_input_consumer* cis);
	void push_to_console(const std::string& text);
	void clear_console();

	void set_script_engine_ptr(script_system* s);

	void show_console();
	void hide_console();
	bool is_console_showed() const;

	gui(const gui&) = delete;
	gui& operator=(const gui&) = delete;
	gui(gui&& o) noexcept;
	gui& operator=(gui&& o) noexcept;
};
