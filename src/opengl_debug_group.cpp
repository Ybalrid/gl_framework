#include "opengl_debug_group.hpp"

#if !defined(NDEBUG)
#include "GL/glew.h"
#include <cstring>
#endif

//Wrap the calls to push/pop debug group in regular functions that will always work
inline void push_opengl_debug_group(const char* name)
{
#if !defined(NDEBUG)
  if(glPushDebugGroup) //likely to fail on macOS without this test, as opengl 4.1 shouldn't have access to this
    glPushDebugGroup(GL_DEBUG_SOURCE_THIRD_PARTY, GLsizei(0), GLsizei(std::strlen(name)), name);
#else
  (void)name;
#endif
}

inline void pop_opengl_debug_group()
{
#if !defined(NDEBUG)
  if(glPopDebugGroup) glPopDebugGroup();
#endif
}

opengl_debug_group::opengl_debug_group(const char* name) { push_opengl_debug_group(name); }
opengl_debug_group::~opengl_debug_group() { pop_opengl_debug_group(); }
