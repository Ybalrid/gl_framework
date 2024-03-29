#ifdef _WIN32
#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#endif

#include "gui.hpp"
#include "resource_system.hpp"
#include "script_system.hpp"
#include <imgui.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>

#include <iostream>
#include "application.hpp"

#define CPP_SDL2_GL_WINDOW
#include <cpp-sdl2/sdl.hpp>

#include "gizmo.hpp"
#include "opengl_debug_group.hpp"

void gui::set_script_engine_ptr(script_system* s) { scripting_engine = s; }


void gui::console()
{
  if(!show_console_) return;

  //Acquire window geometry
  int x, y;
  SDL_GetWindowSize(w, &x, &y);

  //Configure window styling
  ImGui::SetNextWindowSize(ImVec2(float(x), float(std::min(int(y * 0.75f), 900))));
  ImGui::SetNextWindowPos({ 0, 0 });
  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
  ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.025f, 0.025f, 0.025f, 0.75f));
  ImGui::Begin("Console", &show_console_, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoDocking);

  // Leave room for 1 separator + 1 InputText
  ImGui::BeginChild("ScrollingRegion", ImVec2(0, -30), false, ImGuiWindowFlags_HorizontalScrollbar);

  ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

  ImGui::PushFont(console_font);
  for(const auto& log_line : console_content) ImGui::TextUnformatted(log_line.c_str());
  ImGui::PopFont();

  if(scroll_console_to_bottom) ImGui::SetScrollHereY(1.f);
  scroll_console_to_bottom = false;

  ImGui::PopStyleVar();

  ImGui::EndChild();
  ImGui::Separator();
  //---------------
  bool reclaim_focus = false;

  ImGui::PushFont(console_font);
  ImGui::PushItemWidth(-1);
  if(!forced_keyboard_focus_next)
  {
    ImGui::SetKeyboardFocusHere(1);
    forced_keyboard_focus_next = true;
  }

  if(ImGui::InputText(
    "##Input",
    console_input,
    sizeof(console_input),
    ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory,
    [](ImGuiInputTextCallbackData* data) -> int {
      gui* ui = static_cast<gui*>(data->UserData);
      switch(data->EventFlag)
      {
        case ImGuiInputTextFlags_CallbackCompletion:
          if(ui->scripting_engine)
          {
            const std::string current_input = (ui->console_input);

            //Obtain the list of all compleatable words from the scripting engine.
            auto list_of_words = ui->scripting_engine->global_scope_object_names();

            //Find start of the last word in input buffer
            auto last_word_start_char = (current_input.find_last_of(" .()[]-+-/<>~=\"'"));
            std::string last_word;
            if(last_word_start_char != std::string::npos) { last_word = current_input.substr(1 + last_word_start_char); }
            else // try to use the whole input?
            {
              last_word            = current_input;
              last_word_start_char = 0;
            }

            if(!last_word.empty())
            {
              //find matching symbols with the string at the beginning
              std::vector<std::string> matches;
              auto it = list_of_words.begin();
              while(it != list_of_words.end())
              {
                it = std::find_if(it,
                                  list_of_words.end(),
                                  [&](const std::string& str) {
                                    return str.rfind(last_word, 0) == 0; //this return true if str starts with last_word
                                  });
                if(it != list_of_words.end()) matches.emplace_back(*it);
                //Escape the loop now if we haven't found anything
                if(it == list_of_words.end()) break;
                ++it;
              }

              //if something matches
              if(!matches.empty())
                //if there's more than one match
                if(matches.size() != 1)
                {
                  for(const auto& match : matches)
                  {
                    ui->push_to_console(match);
                    ui->scroll_console_to_bottom = true;
                  }
                }
                else
                {
                  const auto& match = matches[0];
                  {
                    //Delete everything up to the one character after the found delimiter
                    data->DeleteChars(last_word_start_char > 0 ? last_word_start_char + 1 : 0,
                                      int(current_input.size()) - last_word_start_char
                                      - (last_word_start_char > 0 ? 1 : 0));
                    //Write the match at the end of the string
                    data->InsertChars(last_word_start_char > 0 ? last_word_start_char + 1 : 0, match.c_str());
                    ui->scroll_console_to_bottom = true;
                  }
                }
            }
          }
          else ui->push_to_console("no completion yet...");
          break;

        case ImGuiInputTextFlags_CallbackHistory: {
          const char* text               = nullptr;
          const auto console_history_max = int(!ui->console_history.empty() ? ui->console_history.size() - 1 : 0);

          if(data->EventKey == ImGuiKey_UpArrow)
          {
            if(!ui->console_history.empty())
              text = ui->console_history[size_t(console_history_max
                    - std::max<int>(
                      0, std::min<int>(console_history_max, ui->history_counter++)))]
                  .c_str();
            ui->history_counter = std::min<int>(console_history_max, ui->history_counter);
          }

          else if(data->EventKey == ImGuiKey_DownArrow)
          {
            if(!ui->console_history.empty())
              text = ui->console_history[size_t(console_history_max
                    - std::min<int>(console_history_max,
                                    std::max<int>(0, ui->history_counter--)))]
                  .c_str();
            ui->history_counter = std::max<int>(0, ui->history_counter);
          }

          if(text)
          {
            data->DeleteChars(0, data->BufTextLen);
            data->InsertChars(0, text);
          }
        }
        break;

        default: break;
      }

      return 0;
    },
    this))
  {
    console_content.push_back("> " + std::string(console_input));
    console_history.emplace_back(console_input);
    history_counter = 0;

    //do something with text here :
    if(console_input_consumer_ptr)
      (*console_input_consumer_ptr)(std::string(console_input));

    //erase text
    console_input[0] = 0;

    reclaim_focus            = true;
    scroll_console_to_bottom = true;
  }
  ImGui::PopItemWidth();
  ImGui::PopFont();

  ImGui::SetItemDefaultFocus();
  if(reclaim_focus) ImGui::SetKeyboardFocusHere(-1);
  ImGui::End();
  ImGui::PopStyleColor();
  ImGui::PopStyleVar();
}

void gui::show_console()
{
  show_console_     = true;
  forced_keyboard_focus_next = false;
}

void gui::hide_console()
{
  show_console_     = false;
  forced_keyboard_focus_next = false;
}

bool gui::is_console_showed() const { return show_console_; }

gui::gui(SDL_Window* window, SDL_GLContext gl_context) : w(window), scripting_engine(nullptr)
{
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

  //ImGui::StyleColorsLight();
  //ImVec4* colors = ImGui::GetStyle().Colors; //TODO change style
  ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
  ImGui_ImplOpenGL3_Init("#version 330 core");

  //Load our preferred GUI fonts from resource_system
  auto vera_mono_ttf = resource_system::get_file("/fonts/VeraMono.ttf");
  auto biolinum_ttf  = resource_system::get_file("/fonts/LinBiolinum_Rah.ttf");
  ImFontConfig biolinum_config;
  ImFontConfig vera_mono_config;

  //Okay, so for some reason, ImGui wants to own the pointer to the font data, and will call "free" on my pointer.
  //So, we're going to stash the ttf file content on the heap, so it will be happy:
  void* biolinum_data      = malloc(biolinum_ttf.size());
  void* vera_mono_ttf_data = malloc(vera_mono_ttf.size());
  assert(biolinum_data);
  assert(vera_mono_ttf_data);
  memcpy(biolinum_data, biolinum_ttf.data(), biolinum_ttf.size());
  memcpy(vera_mono_ttf_data, vera_mono_ttf.data(), vera_mono_ttf.size());

  strcpy(biolinum_config.Name, "biolinum");
  strcpy(vera_mono_config.Name, "vera mono");
  default_font = io.Fonts->AddFontFromMemoryTTF(biolinum_data, int(biolinum_ttf.size()), 18.f, &biolinum_config);
  console_font = io.Fonts->AddFontFromMemoryTTF(vera_mono_ttf_data, int(vera_mono_ttf.size()), 20.0f, &vera_mono_config);
  pixel_font   = io.Fonts->AddFontDefault();

  if(!console_font) std::cerr << "console font is null\n";
  std::cout << "Initialized ImGui " << IMGUI_VERSION << " based gui system\n";
}

gui::~gui()
{
  if(w)
  {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext();

    std::cout << "Deinitialized gui system\n";
  }
}

const char is_imgui[] = "ImGui code";

void gui::frame()
{
  const auto opengl_debug_tag = opengl_debug_group(is_imgui);

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame(w);
  ImGui::NewFrame();

  if(show_console_) console();

  gizmo::begin_frame();
}

void gui::render() const
{
  const auto opengl_debug_tag = opengl_debug_group(is_imgui);

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void gui::handle_event(sdl::Event e) const { ImGui_ImplSDL2_ProcessEvent(reinterpret_cast<SDL_Event*>(&e)); }

void gui::push_to_console(const std::string& text) { console_content.push_back(text); }

void gui::clear_console() { console_content.clear(); }

void gui::move_from(gui& o)
{
  w   = o.w;
  o.w = nullptr;

  console_font = o.console_font;
  default_font = o.default_font;
  pixel_font   = o.pixel_font;
}

gui::gui(gui&& o) noexcept { move_from(o); }

gui& gui::operator=(gui&& o) noexcept
{
  move_from(o);
  return *this;
}

void gui::set_console_input_consumer(console_input_consumer* cis) { console_input_consumer_ptr = cis; }
