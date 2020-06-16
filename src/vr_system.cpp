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
    const auto w = eye_render_target_sizes[i].x;
    const auto h = eye_render_target_sizes[i].y;
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


//Mixed Reality
#ifdef _WIN32
#include "SharedTextureProtocol.h"
#include "gl_dx_interop.hpp"
#include "imgui.h"
camera* vr_system::get_mr_camera() { return mr_camera; }
GLuint vr_system::get_mr_fbo() { return mr_fbo; }
void vr_system::depth_buffer_write_depth_plane()
{
  static bool hide_cliping_plane = true;
  static float z_offset          = 0;
  ImGui::Begin("LIV background/forground separator");
  ImGui::Checkbox("Hide the clipping plane?", &hide_cliping_plane);
  ImGui::SliderFloat("plane Z offset", &z_offset, -2, 2);
  ImGui::End();
  
  if(hide_cliping_plane)
    glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

  glBindFramebuffer(GL_FRAMEBUFFER, mr_fbo);
  mr_depth_buffer_clear();
  GLint old_program;
  glGetIntegerv(GL_CURRENT_PROGRAM, &old_program);
  if(depth_plane_shader == shader_program_manager::invalid_shader) 
      std::cout << "The depth plane shader is invalid!\n"; 
  else
  {
    auto& shader = shader_program_manager::get_from_handle(depth_plane_shader);
    shader.use();

    const glm::vec3 head_position = head_node->get_world_matrix()[3];
    const glm::vec3 camera_position = mr_camera_node->get_world_matrix()[3];
    glm::vec3 head_to_camera_vector = camera_position - head_position;
    head_to_camera_vector.y         = 0;
    head_to_camera_vector           = glm::normalize(head_to_camera_vector);
    const float rotation = atan2(head_to_camera_vector.z, head_to_camera_vector.x);
    glm::mat4 rotation_matrix = glm::rotate(glm::mat4(1.f), glm::half_pi<float>() - rotation, glm::vec3(0.f, 1.f, 0.f));
    transform xform;
    xform.set_position(head_position + glm::vec3(0,0, 0));
    xform.set_orientation(glm::quat(rotation_matrix));
    xform.set_scale(glm::vec3(10000.0f));
    shader.set_uniform(shader::uniform::model, xform.get_model());
    shader.set_uniform(shader::uniform::view, mr_camera->get_view_matrix());
    shader.set_uniform(shader::uniform::projection, mr_camera->get_projection_matrix());
    shader.set_uniform(shader::uniform::debug_float_0, z_offset);
  }
  glBindBuffer(GL_ARRAY_BUFFER, mr_separation_plane_buffer);
  glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
  glEnableVertexAttribArray(0);
  glDisable(GL_CULL_FACE);
  glDrawArrays(GL_TRIANGLES, 0, 6);
  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glEnable(GL_CULL_FACE);
  glUseProgram(old_program);
  glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
}

void vr_system::mr_depth_buffer_clear()
{
  GLuint current_framebuffer;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, (GLint*)&current_framebuffer);
  if(current_framebuffer != mr_fbo)
    glBindFramebuffer(GL_FRAMEBUFFER, mr_fbo);
  glClear(GL_DEPTH_BUFFER_BIT);
  if(current_framebuffer != mr_fbo) glBindFramebuffer(GL_FRAMEBUFFER, current_framebuffer);
}

bool vr_system::is_mr_active() const
{
  return SharedTextureProtocol::IsActive() && (mr_camera != nullptr) && (mr_fbo != invalid_name);
}

bool vr_system::try_start_mr()
{
  if(SharedTextureProtocol::IsActive())
  {
    if(vr_tracking_anchor && gl_dx11_interop::get()->init())
    {
      //Create camera and node:
      mr_camera_node = vr_tracking_anchor->push_child(create_node());
      mr_camera      = mr_camera_node->assign(std::move(camera()));
      const UINT w   = SharedTextureProtocol::GetWidth();
      const UINT h   = SharedTextureProtocol::GetHeight();

      const auto dx11_shared_texture
          = gl_dx11_interop::get()->create_shared_d3d_texture(DXGI_FORMAT_R8G8B8A8_UNORM, { w, h });

      LIV_Texture              = std::get<ID3D11Texture2D*>(dx11_shared_texture);
      LIV_Texture_SharedHandle = std::get<HANDLE>(dx11_shared_texture);

      if(!LIV_Texture) return false;

      glGenTextures(1, &mr_render_texture);
      glGenRenderbuffers(1, &mr_depth);
      glGenFramebuffers(1, &mr_fbo);

      //Configure framebuffer for MR
      glBindFramebuffer(GL_FRAMEBUFFER, mr_fbo);
      glBindTexture(GL_TEXTURE_2D, mr_render_texture);
      //configure and allocate memory for MR texture, and attach to framebuffer
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
      glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, mr_render_texture, 0);

      //Allocate and add depth buffer to MR target
      glBindRenderbuffer(GL_RENDERBUFFER, mr_depth);
      glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
      glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, mr_depth);
      const auto status = glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE;

      glBindFramebuffer(GL_FRAMEBUFFER, 0);

      mr_texture_size.x = w;
      mr_texture_size.y = h;

      if(depth_plane_shader == shader_program_manager::invalid_shader)
      {

        depth_plane_shader = shader_program_manager::create_shader("/shaders/depth-mr.vert", "/shaders/depth-mr.frag");
        glGenBuffers(1, &mr_separation_plane_buffer);
        glBindBuffer(GL_ARRAY_BUFFER, mr_separation_plane_buffer);
        float buffer_data[] {
          -1, 1,
          1, 1,
          -1, -1,
          -1, -1,
          1, 1,
          1, -1
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(buffer_data), buffer_data, GL_STATIC_DRAW);

      }
      return status;
    }
  }
}

void vr_system::submit_to_LIV() const
{
  gl_dx11_interop::get()->copy(
      mr_render_texture, LIV_Texture, { SharedTextureProtocol::GetWidth(), SharedTextureProtocol::GetHeight() });

  SharedTextureProtocol::SubmitTexture(LIV_Texture_SharedHandle);
}

sdl::Vec2i vr_system::get_mr_size() const { return mr_texture_size; }

#endif
