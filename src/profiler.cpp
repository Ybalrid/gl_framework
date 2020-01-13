#include "profiler.hpp"
#include <cpp-sdl2/sdl.hpp>

#include "imgui.h"

profiler* profiler::instance        = nullptr;
bool profiler::show_profiler_window = true;

void profiler::push_tagged_event(frame_event type)
{
  add({ SDL_GetPerformanceCounter(), event_stack::push, type });

  if(type == frame_event::frame) [[unlikely]]
    {
      frame_starts.push_back(samples.size() - 1);
    }
}

void profiler::pop_tagged_event(frame_event type) { add({ SDL_GetPerformanceCounter(), event_stack::pop, type }); }

void profiler::add(sample s) { samples.push_back(s); }

void profiler::draw_profiler_window()
{

  if(show_profiler_window)
  {
    if(ImGui::Begin("Profiler", &show_profiler_window))
    {
      static int frame       = 0;
      static bool use_latest = true;
      ImGui::Text("Profiler settings");
      ImGui::InputInt("Frame", &frame);
      ImGui::Checkbox("Stick to most recent frame", &use_latest);

      if(use_latest) { frame = frame_starts[std::max<long long>(0LL, int(frame_starts.size()) - 2)]; }

      ImGui::End();
    }
  }
}

profiler::profiler()
{
  if(instance) throw std::runtime_error("Cannot create multiple profilers!");
  instance = this;
  samples.reserve(2 * 15 * 120 * 60);
}

profiler::~profiler() { instance = nullptr; }

void profiler::profiler_window() { instance->draw_profiler_window(); }

profiler::tag::tag(frame_event event_type) : type { event_type } { profiler::instance->push_tagged_event(type); }

profiler::tag::~tag() { profiler::instance->pop_tagged_event(type); }
