#pragma once
#include "vr_system.hpp"
#if USING_OPENXR

//Configure XR headers
#define XR_USE_GRAPHICS_API_OPENGL
#ifdef WIN32
#define XR_USE_GRAPHICS_API_D3D11
#define XR_USE_PLATFORM_WIN32
#include "gl_dx_interop.hpp"
#else
#define XR_USE_PLATFORM_XLIB
#include <GL/glew.h>
#include <GL/glx.h>
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>

#include <vector>

class vr_system_openxr : public vr_system
{
  std::vector<const char*> enabled_extension_properties_names;
  XrInstance instance      = XR_NULL_HANDLE;
  XrSession session        = XR_NULL_HANDLE;
  XrSystemId system_id     = 0;
  XrSwapchain swapchain[2] = { 0 };
  XrViewConfigurationType used_view_configuration_type;
  std::vector<XrView> views;
  std::vector<XrSwapchainImageOpenGLKHR> swapchain_images_opengl[2];
#ifdef _WIN32
  std::vector<XrSwapchainImageD3D11KHR> swapchain_images_d3d11[2];
#endif
  XrFrameState current_frame_state;
  XrCompositionLayerBaseHeader* layers[1];
  XrCompositionLayerProjectionView projection_layer_views[2];
  XrSpace application_space = XR_NULL_HANDLE;

  node* eye_camera_node[2] = { nullptr, nullptr };

#ifdef WIN32
  gl_dx11_interop* dx11_interop;
  bool fallback_to_dx = false;
#endif

  public:
  static bool need_to_vflip;

  vr_system_openxr();
  virtual ~vr_system_openxr();

  bool initialize(sdl::Window& window) final;
  void build_camera_node_system() final;
  void wait_until_next_frame() final;
  void update_tracking() final;
  void submit_frame_to_vr_system() final;

  [[nodiscard]] virtual bool must_vflip() const final { return need_to_vflip; }

#ifdef WIN32
  void update_mr_camera() final {};
#endif
};
#endif
