#include "profiler.hpp"
#include <cpp-sdl2/sdl.hpp>
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

void profiler::draw_profiler_window() { }

profiler::profiler()
{
  if(instance) throw std::runtime_error("Cannot create multiple profilers!");
  instance = this;
  samples.reserve(2 * 15 * 120 * 5);
}

profiler::~profiler() { instance = nullptr; }

void profiler::profiler_window() { instance->draw_profiler_window(); }

profiler::tag::tag(frame_event event_type) : type { event_type } { profiler::instance->push_tagged_event(type); }

profiler::tag::~tag() { profiler::instance->pop_tagged_event(type); }
