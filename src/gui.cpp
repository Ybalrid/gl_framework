#include "gui.hpp"
#include <imgui.h>
#include <examples/imgui_impl_sdl.h>
#include <examples/imgui_impl_opengl3.h>

#define CPP_SDL2_GL_WINDOW
#include <cpp-sdl2/sdl.hpp>
void gui::console()
{
	ImGui::SetNextWindowSize(ImVec2(500, 650), ImGuiCond_FirstUseEver);
	if (ImGui::Begin("Console", &show_console))
	{
		ImGui::BeginChild("ScrollingRegion", ImVec2(0, -30), false, ImGuiWindowFlags_HorizontalScrollbar); // Leave room for 1 separator + 1 InputText
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1));

		for (const auto& log_line : console_content)
			ImGui::TextUnformatted(log_line.c_str());

		if (scroll_console_to_bottom)
			ImGui::SetScrollHereY(1.f);
		scroll_console_to_bottom = false;

		ImGui::PopStyleVar();
		ImGui::EndChild();
		ImGui::Separator();
		//---------------
		bool reclaim_focus = false;
		if(ImGui::InputText("Input", console_input, IM_ARRAYSIZE(console_input), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_CallbackCompletion | ImGuiInputTextFlags_CallbackHistory, 
			[](ImGuiInputTextCallbackData* data) -> int
		{
			gui* ui = static_cast<gui*>(data->UserData);
			switch(data->EventFlag)
			{
			case ImGuiInputTextFlags_CallbackHistory:
				const char* text = nullptr;
				if (data->EventKey == ImGuiKey_UpArrow)
				{
					if (!ui->console_history.empty())
						text = ui->console_history[ui->console_history.size() - 1 - std::max<int>(0,  std::min<int>(ui->console_history.size() - 1, ui->history_counter++))].c_str();
					ui->history_counter = std::min<int>(ui->console_history.size() - 1, ui->history_counter);
				}
				else if (data->EventKey == ImGuiKey_DownArrow)
				{
					if (!ui->console_history.empty())
						text = ui->console_history[ui->console_history.size() - 1 - std::min<int>(ui->console_history.size() - 1, std::max<int>(0,ui->history_counter--))].c_str();
					ui->history_counter = std::max<int>(0, ui->history_counter);
				}

				if (text)
				{
					data->DeleteChars(0, data->BufTextLen);
					data->InsertChars(0, text);
				}

				break;

			}

			return 0;
		}, this))
		{
			console_content.push_back("> " + std::string(console_input));
			console_history.emplace_back(console_input);
			history_counter = 0;

			//do something with text here : 
			if(cis_ptr)
				(*cis_ptr)(std::string(console_input));

			//erase text
			console_input[0] = 0;

			reclaim_focus = true;
			scroll_console_to_bottom = true;
		}

		ImGui::SetItemDefaultFocus();
		if (reclaim_focus)
			ImGui::SetKeyboardFocusHere(-1);
	}
	ImGui::End();
}

gui::gui(SDL_Window* window, SDL_GLContext gl_context)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window, gl_context);
	ImGui_ImplOpenGL3_Init("#version 330");

	w = window;
}

gui::~gui()
{
	if (w)
	{
		ImGui_ImplOpenGL3_Shutdown();
		ImGui_ImplSDL2_Shutdown();
		ImGui::DestroyContext();
	}
}

void gui::frame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(w);
	ImGui::NewFrame();

	if (show_console)
		console();
}

void gui::render()
{
	ImGui::Render();
	ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void gui::handle_event(sdl::Event e)
{
	ImGui_ImplSDL2_ProcessEvent(reinterpret_cast<SDL_Event*>(&e));
}

void gui::push_to_console(const std::string &text)
{
	console_content.push_back(text);
}

void gui::clear_console()
{
	console_content.clear();
}

void gui::set_console_input_consumer(console_input_consumer *cis)
{
	cis_ptr = cis;
}
