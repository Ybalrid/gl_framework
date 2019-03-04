#include "gui.hpp"

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
			switch(data->EventFlag)
			{
			case ImGuiInputTextFlags_CallbackHistory:
				//TODO handle command history;
				break;
			}

			return 0;
		}, this))
		{
			console_content.push_back("> " + std::string(console_input));

			//do something with text here : 
			
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

gui::gui(sdl::Window& window, sdl::Window::GlContext& gl_context)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window.ptr(), gl_context.ptr());
	ImGui_ImplOpenGL3_Init("#version 330");

	w = window.ptr();
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
