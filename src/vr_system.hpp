#pragma once
#include <cpp-sdl2/sdl.hpp>
#include "build_config.hpp"

#include <memory>
#include "node.hpp"
#include <glm/glm.hpp>

#ifdef _WIN32
#include "gl_dx_interop.hpp"
#endif

#include <atomic>
#include <mutex>
#include <thread>

#include "shader_program_manager.hpp"

//This is the interface for all VR systems
class vr_system
{
  protected:
  constexpr static GLuint invalid_name { std::numeric_limits<GLuint>::max() };

  ///Any VR system needs a reference point in the scene to sync tracking between real and virtual world
  node* vr_tracking_anchor = nullptr;
  ///Pair or camera objects to render in stereoscopy
  camera* eye_camera[2] = { nullptr, nullptr };

  ///Framebuffer objects to render to texture
  GLuint eye_fbo[2] = { invalid_name, invalid_name };
  ///Render texture
  GLuint eye_render_texture[2] = { invalid_name, invalid_name };
  ///Render buffer for depth
  GLuint eye_render_depth[2] = { invalid_name, invalid_name };

  ///Size of the above buffers in pixels
  sdl::Vec2i eye_render_target_sizes[2] {};

  node* head_node    = nullptr;
  node* hand_node[2] = { nullptr, nullptr };

#ifdef _WIN32
  node* mr_camera_node              = nullptr;
  camera* mr_camera                 = nullptr;
  GLuint mr_fbo                     = invalid_name;
  GLuint mr_depth                   = invalid_name;
  GLuint mr_render_texture          = invalid_name;
  GLuint mr_separation_plane_buffer = invalid_name;
  sdl::Vec2i mr_texture_size        = {};

  shader_handle depth_plane_shader = shader_program_manager::invalid_shader;

  //LIV specific
  ID3D11Texture2D* LIV_Texture;
  HANDLE LIV_Texture_SharedHandle;

  //Directory watcher to read FOV updates
  bool fov_from_file                      = false;
  static constexpr char* config_file_name = "ExternalCamera.cfg";
  HMODULE exe_module                      = NULL;
  char exe_dir_path[MAX_PATH];
  HANDLE exe_dir_change_notification;
  std::mutex fov_mutex;
  std::atomic_bool watcher_alive;
  std::thread watcher;
  std::unordered_map<std::string, std::string> configuration;

#endif

  bool initialized_opengl_resources = false;

  public:
  //Nothing special to do in ctor/dtor
  vr_system() = default;
  virtual ~vr_system();

  enum class eye { left = 0, right = 1 };

  //--- the following is generic and should work as-is for all vr systems
  ///Get the eye frame buffers
  GLuint get_eye_framebuffer(eye);
  ///Get the size of the framebuffers in pixel
  sdl::Vec2i get_eye_framebuffer_size(eye);
  ///Set this pointer to become the root of the tracked object's sub-tree
  void set_anchor(node* node);
  ///get the camera to use for rendering
  camera* get_eye_camera(eye output);

  //--- the following is the abstract interface that require implementation specific work
  ///Call this once OpenGL is fully setup. This function ini the VR system, and populate the `eye_render_target_sizes`.
  ///Please call initialize_opengl_resources() once this has run to create the eye render buffers
  virtual bool initialize(sdl::Window& window) = 0;
  ///Only call this after `set_anchor` to a non-null pointer
  virtual void build_camera_node_system() = 0;
  ///Needs to be called before any rendering, sync with the headset. On some platform, this also acquire tracking states
  virtual void wait_until_next_frame() = 0;
  ///Move scene nodes to match VR tracking
  virtual void update_tracking() = 0;
  ///Send content of the framebuffers to the VR system
  virtual void submit_frame_to_vr_system() = 0;
  ///Return true if this VR system will peform an image vflip in the projection matrix. This changes face culling order.
  [[nodiscard]] virtual bool must_vflip() const = 0;

  void initialize_opengl_resources();

  //Mixed Reality
#ifdef _WIN32
  bool is_mr_active() const;
  bool try_start_mr();
  camera* get_mr_camera() const;
  GLuint get_mr_fbo() const;
  void depth_buffer_write_depth_plane() const;
  void mr_depth_buffer_clear() const;
  void submit_to_LIV() const;
  sdl::Vec2i get_mr_size() const;

  void subscribe_directory_watcher();
  void read_configuration();
  void update_fov();

  float get_mr_fov();

  //Interface for underlying VR system
  virtual void update_mr_camera() = 0;

#endif
};

using vr_system_ptr = std::unique_ptr<vr_system>;

#if USING_OPENVR
//#include openvr based implementation of above interface
#include "vr_system_openvr.hpp"
#endif

#if USING_OCULUS_VR
//#include ovr based implementation of above interface
#include "vr_system_oculus.hpp"
#endif

#if USING_OPENXR
#include "vr_system_openxr.hpp"
#endif
