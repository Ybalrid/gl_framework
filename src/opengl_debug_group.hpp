#pragma once 
/// RAII wrap the push/pop of OpenGL debug group names
struct opengl_debug_group
{
  opengl_debug_group(const char* name);
  ~opengl_debug_group();
};