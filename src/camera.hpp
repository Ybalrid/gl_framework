#pragma once

#include "transform.hpp"

#include <array>

class camera
{
  public:
  static int last_w, last_h, last_x, last_y;

  ///parameters that can be changed at will
  enum projection_mode { perspective, orthographic, hud, vr_eye_projection };

  ///By default, the projection mode of a camera is perspective
  projection_mode projection_type { perspective };

  ///This function pointer is a callback to be used for feeding the projection matrix from either a VR runtime (SteamVR, Oculus, or OpenXR)
  void (*vr_eye_projection_callback)(glm::mat4& projection_output, float near_clip, float far_clip) = nullptr;

  float near_clip = 0.25f;
  float far_clip  = 100.f;
  float fov       = 45.f;

  ///Matrix that moves the world so that the camera look at it how you want
  [[nodiscard]] glm::mat4 get_view_matrix() const;
  ///Matrix that move everything into a [-1;1] cube
  [[nodiscard]] glm::mat4 get_projection_matrix() const;
  ///Multiplication of the projection and the view matrices
  [[nodiscard]] glm::mat4 get_view_projection_matrix() const;
  ///Set the global transform (model matrix) of the camera
  void set_world_matrix(const glm::mat4& matrix);
  ///Set the view matrix of the camera (it actually set the model matrix internally)
  void set_view_matrix(const glm::mat4& matrix);
  ///Call this with the viewport geometry
  void update_projection(int viewport_w, int viewport_h, int viewport_x = 0, int viewport_y = 0);

  private:
  //enclosed projection matrix
  glm::mat4 projection { 1.f };
  ///We only store a model matrix, the view matrix is computed by inveting the model matrix
  glm::mat4 world_model_matrix { 1.f };
  static void set_gl_viewport(int x, int y, int w, int h);
};
