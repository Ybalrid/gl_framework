#include "gui.hpp"

void dummy()
{

}

gui::gui(sdl::Window& window, sdl::Window::GlContext& gl_context)
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	(void)io;

	ImGui::StyleColorsDark();
	ImGui_ImplSDL2_InitForOpenGL(window.ptr(), gl_context.ptr());
	ImGui_ImplOpenGL3_Init("#version 330");

	w = window.ptr();
}

gui::~gui()
{
	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();
}

void gui::frame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplSDL2_NewFrame(w);
	ImGui::NewFrame();
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
