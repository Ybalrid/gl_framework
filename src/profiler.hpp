#pragma once

#include <thread>
#include <vector>
#include <cstdint>

enum class frame_event {
  frame,
  frame_prepare,
  events,
  render,
  vr_tracking,
  gui_render,
  build_draw_list,
  cull_test,
  draw_oob,
  render_draw_list,
  render_shadow_map,

};

enum class event_stack { push, pop };

class profiler
{
  static profiler* instance;

  struct sample
  {
    uint64_t timestamp;
    event_stack stack;
    frame_event type;
  };

  static constexpr size_t sizeof_sample = sizeof(sample);

  void push_tagged_event(frame_event type);
  void pop_tagged_event(frame_event type);
  void add(sample s);

  std::vector<sample> samples;

  void draw_profiler_window();

  std::vector<size_t> frame_starts;

  public:
  struct tag
  {
    const frame_event type;

    tag(frame_event event_type);
    ~tag();
  };

  friend struct tag;
  profiler();
  ~profiler();

  static void profiler_window();
  static bool show_profiler_window;
};
