#include "opengl_debug_group.hpp"

#include "GL/glew.h"
#include <cstring>

//Wrap the calls to push/pop debug group in regular functions that will always work
inline void push_opengl_debug_group(const char* name)
{
  if(glPushDebugGroup) //likely to fail on macOS without this test, as opengl 4.1 shouldn't have access to this
    glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, GLsizei(0), GLsizei(std::strlen(name)), name);
}

inline void pop_opengl_debug_group()
{
  if(glPopDebugGroup) glPopDebugGroup();
}

opengl_debug_group::opengl_debug_group(const char* name) { push_opengl_debug_group(name); }
opengl_debug_group::~opengl_debug_group() { pop_opengl_debug_group(); }
