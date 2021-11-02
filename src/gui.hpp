#pragma once

#include <GL/glew.h>

#include <vector>
#include <string>

///Forward declare some SDL types
namespace sdl
{
  class Window;
  union Event;
}
struct SDL_Window;
typedef void* SDL_GLContext;
///Forward declare some other types
struct ImFont;
class script_system;

///Interface to get text typed from inside the console
struct console_input_consumer
{
  virtual ~console_input_consumer()               = default;
  virtual bool operator()(const std::string& str) = 0;
};

///Our ImGui based GUI system
class gui
{
  SDL_Window* w = nullptr;

  ///Display the console
  void console();

  ///Input buffer ( = text you type )
  char console_input[2048] = { 0 };

  ///Content of the console displayed on screen in scrollable region
  std::vector<std::string> console_content {};
  ///Console history, list of commands that have been ran in the past
  std::vector<std::string> console_history {};
  ///When this flag is set to true, next call of gui::console will stick the scrollbar at the bottom
  bool scroll_console_to_bottom = false;
  ///Currently used input consumer
  console_input_consumer* console_input_consumer_ptr = nullptr;
  ///Number of things we are down the history right now
  int history_counter = 0;

  ///Font for the console (monospaced)
  ImFont* console_font;
  ///Font for the UI (normal)
  ImFont* default_font;
  ///Default ImGui font
  ImFont* pixel_font;

  ///Move utility
  void move_from(gui& other);

  ///Pointer to the scripting engine
  script_system* scripting_engine;

  bool show_console_     = false;
  bool last_frame_showed = false;

  public:
  gui() = default;

  ///Initialize the gui system
  gui(SDL_Window* window, SDL_GLContext gl_context);
  ~gui();

  ///Call it to start each frame
  void frame();
  ///Call it to render on screen
  void render() const;
  ///Give events to the GUI
  void handle_event(sdl::Event e) const;
  ///Set a pointer to who uses the console text
  void set_console_input_consumer(console_input_consumer* cis);
  ///Add text to the console on display
  void push_to_console(const std::string& text);
  ///Erase the console on screen
  void clear_console();
  ///Give pointer to the scripting engine
  void set_script_engine_ptr(script_system* s);
  ///Show the console
  void show_console();
  ///Hide the console
  void hide_console();
  ///Return true if console is on screen
  bool is_console_showed() const;
  ///No copy
  gui(const gui&) = delete;
  ///No copy
  gui& operator=(const gui&) = delete;
  ///Move
  gui(gui&& o) noexcept;
  ///Move
  gui& operator=(gui&& o) noexcept;
};
