#include "vr_system.hpp"

vr_system::~vr_system()
{
  if(initialized_opengl_resources)
  {
    glDeleteFramebuffers(2, eye_fbo);
    glDeleteRenderbuffers(2, eye_render_depth);
    glDeleteTextures(2, eye_render_texture);
  }
}

GLuint vr_system::get_eye_framebuffer(eye output) { return eye_fbo[(size_t)output]; }

sdl::Vec2i vr_system::get_eye_framebuffer_size(eye output) { return eye_render_target_sizes[(size_t)output]; }

void vr_system::set_anchor(node* node)
{
  assert(node);
  vr_tracking_anchor = node;
}

camera* vr_system::get_eye_camera(eye output) { return eye_camera[(size_t)output]; }

void vr_system::initialize_opengl_resources()
{
  //OpenGL resource initialization

  //The rest of the engine don't care bout our "VR" hardware.
  //It just want to bind and render to a pair of FBOs, one for left eye, one for right
  glGenTextures(2, eye_render_texture);
  glGenRenderbuffers(2, eye_render_depth);
  glGenFramebuffers(2, eye_fbo);

  for(size_t i = 0; i < 2; ++i)
  {
    auto w = eye_render_target_sizes[i].x;
    auto h = eye_render_target_sizes[i].y;
    //Configure textures
    glBindFramebuffer(GL_FRAMEBUFFER, eye_fbo[i]);
    glBindTexture(GL_TEXTURE_2D, eye_render_texture[i]);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eye_render_texture[i], 0);

    glBindRenderbuffer(GL_RENDERBUFFER, eye_render_depth[i]);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, eye_render_depth[i]);

    if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    { std::cerr << "eye fbo " << i << " is not complete" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << "\n"; }
  }
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  initialized_opengl_resources = true;
}
