#include "application.hpp"
#include "image.hpp"
#include "shader.hpp"
#include "renderable.hpp"
#include "camera.hpp"
#include "gui.hpp"
#include "scene_object.hpp"
#include "gltf_loader.hpp"
#include "imgui.h"
#include <cpptoml.h>
#include "light.hpp"
#include <chrono>

#include "opengl_debug_group.hpp"
#include "gizmo.hpp"


//TODO move these globals to the application class!
float shadow_map_ortho_scale          = 80;
float shadow_map_direction_multiplier = 95;
float shadow_map_near_plane           = .337;
float shadow_map_far_plane            = 300;
glm::vec3 sun_direction_unormalized { -0.25f, -1.f, -0.10f };
GLuint bbox_drawer_vbo, bbox_drawer_ebo, bbox_drawer_vao;
GLuint line_drawer_vbo, line_drawer_vao;
shader_handle color_debug_shader = shader_program_manager::invalid_shader;
shader_handle vertex_debug_shader = shader_program_manager::invalid_shader;

//The statics
std::vector<std::string> application::resource_paks;
scene* application::main_scene = nullptr;
glm::vec4 application::clear_color { 0.4f, 0.5f, 0.6f, 1.f };


void application::activate_vsync() const
{
  try
  {
    sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::adaptive_vsync);
  }
  catch(const sdl::Exception& cannot_adaptive)
  {
    std::cerr << cannot_adaptive.what() << '\n';
    std::cerr << "Using standard vsync instead of adaptive vsync\n";
    try
    {
      sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::vsync);
    }
    catch(const sdl::Exception& cannot_standard_vsync)
    {
      std::cerr << cannot_standard_vsync.what() << '\n';
      std::cerr << "Cannot set vsync for this driver.\n";
      try
      {
        sdl::Window::gl_set_swap_interval(sdl::Window::gl_swap_interval::immediate);
      }
      catch(const sdl::Exception& cannot_vsync_at_all)
      {
        std::cerr << cannot_vsync_at_all.what() << '\n';
      }
    }
  }
}


void scene_node_outline(node* current_node, node*& active_node)
{
  std::string type;
  current_node->visit([&](auto&& content) {
    if constexpr(std::is_same_v<std::decay_t<decltype(content)>, std::monostate>)
      type = "empty";
    else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, scene_object>)
      type = "scene_object";
    else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, camera>)
      type = "camera";
    else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, light>)
      type = "light";
    else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, point_light>)
      type = "point_light";
    else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, audio_source>)
      type = "audio_source";
    else if constexpr(std::is_same_v<std::decay_t<decltype(content)>, listener_marker>)
      type = "audio_listener_marker";
    else
      type = std::string("ERROR: missing node type in") + __FILE__ + " " + std::to_string(__LINE__);
  });
  const std::string name = type + " node " + std::to_string(current_node->get_id()) + (!current_node->get_name().empty() ? " [" + current_node->get_name() + "]" : "") ;

  //Draw selection ball
  if(ImGui::RadioButton(("###select " + name).c_str(), current_node == active_node)) active_node = current_node;
  ImGui::SameLine();
  if(ImGui::TreeNode(name.c_str()))
  {
    current_node->visit([&](auto&& content) {
      using T = std::decay_t<decltype(content)>;
      if constexpr(std::is_same<T, point_light>::value)
      {
        auto& light = static_cast<point_light&>(content);
        ImGui::ColorEdit3("Diffuse", glm::value_ptr(light.diffuse));
        ImGui::ColorEdit3("Ambient", glm::value_ptr(light.ambient));
        ImGui::SliderFloat("Constant", &light.constant, 0, 5);
        ImGui::SliderFloat("Linear", &light.linear, 0, 2);
        ImGui::SliderFloat("Quadratic", &light.quadratic, 0, 2);
        ImGui::Separator();
      }
    });

    const auto children_count = current_node->get_child_count();
    for(size_t i = 0; i < children_count; ++i) { scene_node_outline(current_node->get_child(i), active_node); }
    ImGui::TreePop();
  }
}

void application::draw_debug_ui()
{
  static auto show_demo_window  = false;
  static auto show_style_editor = false;

  if(debug_ui)
  {
    sdl::Mouse::set_relative(false);

    if(ImGui::Begin("Texture Manager state", &debug_ui))
    {
      const auto& textures      = texture_manager::get_list();
      const auto texture_number = textures.size();
      ImGui::Text("Loaded %lu textures", texture_number);
      for(size_t i = 0; i < texture_number; ++i)
      {
        std::string header_name
            = i == texture_manager::get_dummy_texture() ? "dummy texture" : "texture[" + std::to_string(i) + "]";
        if(ImGui::CollapsingHeader(header_name.c_str()))
        {
          const auto& texture = textures[i];
          const GLuint id     = texture.get_glid();
          ImGui::Text("Texture OpenGL ID = %d", id);
          ImTextureID ImGuiTextureHandle = nullptr;

          //gracefully handle the widening of the value in 64bit
          ImGui::Image(ImTextureID(size_t(id)), ImVec2(256.f, 256.f), ImVec2(0, 1), ImVec2(1, 0));
        }
      }
    }
    ImGui::End();

    if(ImGui::Begin("Renderable Manager State", &debug_ui))
    {
      const auto& list             = renderable_manager::get_list();
      const auto renderable_number = list.size();
      ImGui::Text("Loaded %lu renderables", renderable_number);
      if(ImGui::CollapsingHeader("Content"))
        for(size_t i = 0; i < renderable_number; i++)
        {
          const auto& renderable        = list[i];
          std::string renderable_header = "renderable[" + std::to_string(i) + "]";
          if(ImGui::CollapsingHeader(renderable_header.c_str()))
          {
            ImGui::Text("Material : shinyness %f", renderable.mat.shininess);
            ImGui::ColorEdit3("diffuse", const_cast<float*>(glm::value_ptr(renderable.mat.diffuse_color)));
            ImGui::ColorEdit3("specular", const_cast<float*>(glm::value_ptr(renderable.mat.specular_color)));
            const auto bounds = renderable.get_bounds();
            ImGui::TextWrapped("(Model space) Bounds : min(%f, %f, %f) max(%f, %f, %f)",
                               bounds.min.x,
                               bounds.min.y,
                               bounds.min.z,
                               bounds.max.x,
                               bounds.max.y,
                               bounds.max.z);
            const auto diffuse  = renderable.get_diffuse_texture();
            const auto specular = renderable.get_specular_texture();
            const auto normal   = renderable.get_normal_texture();

            ImGui::Text("Textures:");
            ImGui::Text("Diffuse : %lu", diffuse == texture_mgr.invalid_texture ? -1 : diffuse);
            ImGui::Text("Specular : %lu", specular == texture_mgr.invalid_texture ? -1 : specular);
            ImGui::Text("Normal : %lu", normal == texture_mgr.invalid_texture ? -1 : normal);
          }
        }
    }
    ImGui::End();

    if(ImGui::Begin("Debugger Window", &debug_ui))
    {
      ImGui::Text("FPS: %d", fps);
      ImGui::Text("%3lu objects passed frustum culling", draw_list.size());
      ImGui::Separator();
      ImGui::Checkbox("Draw physics debug ?", &debug_draw_physics);
      ImGui::Checkbox("Show *all* object's bounding boxes?", &debug_draw_bbox);
      ImGui::Checkbox("Show ImGui demo window ?", &show_demo_window);
      ImGui::Checkbox("Show ImGui style editor ?", &show_style_editor);
      //ImGui::BeginChild(
      //	"##debugger window scrollable region", ImVec2(300, 500), true, ImGuiWindowFlags_AlwaysVerticalScrollbar);
      ImGui::Separator();
      ImGui::SliderFloat3("sun direction", static_cast<float*>(glm::value_ptr(sun_direction_unormalized)), -1.f, 1.f);
      ImGui::ColorEdit3("Sun color", glm::value_ptr(sun.diffuse));
      ImGui::Separator();
      ImGui::Text("Main ShadowMap:");
      ImGui::SliderFloat("near", &shadow_map_near_plane, 0.0001f, 1.f);
      ImGui::SliderFloat("far", &shadow_map_far_plane, 50.f, 500.f);
      ImGui::SliderFloat("ortho window", &shadow_map_ortho_scale, 10.f, 200.f);
      ImGui::SliderFloat("distance scale", &shadow_map_direction_multiplier, 1.f, 1000.f);
      ImGui::Image(ImTextureID(size_t(shadow_depth_map)), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));
    }
    ImGui::End();

    static ImGuizmo::OPERATION operation = ImGuizmo::OPERATION::TRANSLATE;
    static ImGuizmo::MODE mode           = ImGuizmo::MODE::WORLD;
    static node* active_node             = nullptr;

    if(ImGui::Begin("Transform mode", &debug_ui))
    {
      ImGui::Text("Operation");
      if(ImGui::RadioButton("Translate", operation == ImGuizmo::TRANSLATE)) operation = ImGuizmo::TRANSLATE;
      ImGui::SameLine();
      if(ImGui::RadioButton("Rotate", operation == ImGuizmo::ROTATE)) operation = ImGuizmo::ROTATE;
      ImGui::SameLine();
      if(ImGui::RadioButton("Scale", operation == ImGuizmo::SCALE)) operation = ImGuizmo::SCALE;
      ImGui::Separator();
      ImGui::Text("Mode");
      if(ImGui::RadioButton("World", mode == ImGuizmo::WORLD)) mode = ImGuizmo::WORLD;
      ImGui::SameLine();
      if(ImGui::RadioButton("Local", mode == ImGuizmo::LOCAL)) mode = ImGuizmo::LOCAL;
    }
    ImGui::End();

    if(ImGui::Begin("nodes", &debug_ui))
    {
      if(ImGui::RadioButton("Noting selected", active_node == nullptr)) active_node = nullptr;

      scene_node_outline(main_scene->scene_root.get(), active_node);
    }
    ImGui::End();

    if(active_node) { gizmo::manipulate(active_node, main_camera, mode, operation); }

    if(show_style_editor)
    {
      if(ImGui::Begin("Style Editor", &show_style_editor)) ImGui::ShowStyleEditor();
      ImGui::End();
    }

    if(ImGui::Begin("BGM", &debug_ui))
    {
      auto* source = audio.get_bgm_source();
      if(source)
      {
        const auto source_state = audio_source::state_to_string(source->get_state());
        ImGui::Text("Current state %s", source_state.c_str());
        ImGui::Text("Playback position:");
        ImGui::SameLine();
        ImGui::ProgressBar(source->get_playback_position());

        bool looping_state = source->get_looping();
        ImGui::Checkbox("Looping", &looping_state);
        source->set_looping(looping_state);

        float volume = source->get_volume();
        ImGui::SliderFloat("Volume", &volume, 0, 1);
        source->set_volume(volume);

        float pitch = source->get_pitch();
        ImGui::SliderFloat("Pitch", &pitch, 0, 2);
        source->set_pitch(pitch);

        if(ImGui::Button("Play")) source->play();
        ImGui::SameLine();
        if(ImGui::Button("Pause")) source->pause();
        ImGui::SameLine();
        if(ImGui::Button("Stop")) source->stop();
      }
      else
      {
        ImGui::Text("No BGM set.");
      }
    }
    ImGui::End();

    if(show_demo_window) ImGui::ShowDemoWindow(&show_demo_window);
  }

#ifndef NON_NAGGING_DEBUG
#ifdef _DEBUG
  const int x = 400, y = 75;
  ImGui::SetNextWindowPos({ 0, float(window.size().y - y) });
  ImGui::SetNextWindowSize({ float(x), float(y) });
  if(ImGui::Begin(
         "Development build", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground))
  {
    ImGui::Text("This program is a development Debug Build.");
#if defined(USING_JETLIVE)
    ImGui::Text("Dynamic recompilation is available via jet-live.");
    ImGui::Text("Change a source file, and hit Ctrl+R to hotload.");
#elif defined(WIN32)
    ImGui::Text("Dynamic hot-reload of code is available via blink.");
    ImGui::Text("Attach blink to pid %d to hot-reload changed code.", _getpid());
#endif //dynamic hotreloader
  }
  ImGui::End();
#endif //_DEBUG
#endif //NON_NAGGING_DEBUG
}

void application::update_timing()
{
  //calculate frame timing
  last_frame_time      = current_time;
  current_time         = SDL_GetTicks();
  current_time_in_sec  = float(current_time) * .001f;
  last_frame_delta     = current_time - last_frame_time;
  last_frame_delta_sec = float(last_frame_delta) * .001f;

  //take care of the FPS counter
  if(current_time - last_second_time >= 1000)
  {
    fps                   = frames_in_current_sec;
    frames_in_current_sec = 0;
    last_second_time      = current_time;
  }
  frames_in_current_sec++;
}

void application::set_opengl_attribute_configuration(const bool multisampling,
                                                     const int samples,
                                                     const bool srgb_framebuffer) const
{
  const auto major = 4;
#ifndef __APPLE__
  const auto minor = 3;
#else
  std::cout << "On MacOS we only use OpenGL 4.1 instead of 4.3. A few features (mostly debug) will not work.\n";
  const auto minor = 1; //Apple actively </3 OpenGL :'-(
#endif

  //Explicitly request a depth buffer of at least 24 bits, instead of the 16 bits by default SDL metion in documetation
  sdl::Window::gl_set_attribute(SDL_GL_DEPTH_SIZE, 24);
  sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLEBUFFERS, multisampling);
  sdl::Window::gl_set_attribute(SDL_GL_MULTISAMPLESAMPLES, samples);
  sdl::Window::gl_set_attribute(SDL_GL_FRAMEBUFFER_SRGB_CAPABLE,
                                srgb_framebuffer); //Fragment shaders will perform individual gamma correction
  sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE); //OpenGL core profile
  sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MAJOR_VERSION, major);                      //OpenGL 4+
  sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_MINOR_VERSION, minor);                      //OpenGL 4.1 or 4.3
#ifdef _DEBUG
  sdl::Window::gl_set_attribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG); //create a so-called "debug" context
#endif
}

void application::initialize_glew() const
{
  if(const auto state = glewInit(); state != GLEW_OK)
  {
    std::cerr << "cannot init glew\n";
    std::cerr << glewGetErrorString(state);
    abort();
  }
  std::cout << "Initialized GLEW " << glewGetString(GLEW_VERSION) << '\n';
}

void application::install_opengl_debug_callback() const
{
#ifdef _DEBUG
  if(glDebugMessageCallback)
  {
    glEnable(GL_DEBUG_OUTPUT);
    glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
    glDebugMessageCallback(
        [](GLenum source,
           GLenum type,
           GLuint id,
           GLenum severity,
           GLsizei /*length*/,
           const GLchar* message,
           const void* /*user_param*/) {
          if(severity == GL_DEBUG_SEVERITY_NOTIFICATION) return;
          std::cerr << "-----\n";
          std::cerr << "opengl debug message: source(" << (source) << ") type(" << (type) << ") id(" << id << ") message("
                    << std::string(message) << ")\n";
          std::cerr << "-----" << std::endl; //flush here
        },
        nullptr);
  }
#endif
}

void application::configure_and_create_window(const std::string& application_name)
{
  //load config
  auto configuration_data = resource_system::get_file("/config.toml");
  configuration_data.push_back('\0'); //add a string terminator
  const std::string configuration_text(reinterpret_cast<const char*>(configuration_data.data()));
  std::istringstream configuration_stream(configuration_text);
  auto config_toml               = cpptoml::parser(configuration_stream);
  const auto loaded_config       = config_toml.parse();
  const auto configuration_table = loaded_config->get_table("configuration");

  //extract config values
  const auto multisampling    = configuration_table->get_as<bool>("multisampling").value_or(true);
  const auto samples          = configuration_table->get_as<int>("samples").value_or(1);
  const auto srgb_framebuffer = false; //nope, sorry. Shader will take care of gamma correction ;)
  const auto hidpi            = configuration_table->get_as<bool>("hidpi").value_or(false);

  //extract window config
  set_opengl_attribute_configuration(multisampling, samples, srgb_framebuffer);
  const auto fullscreen = configuration_table->get_as<bool>("fullscreen").value_or(false);
  sdl::Vec2i window_size;

  const auto window_size_array = configuration_table->get_array_of<int64_t>("resolution");
  window_size.x                = int(window_size_array->at(0));
  window_size.y                = int(window_size_array->at(1));

  const auto level_list = configuration_table->get_array_of<std::string>("levels");
  if(level_list->empty()) { start_level_name = "sponza"; }
  else
  {
    start_level_name = level_list->at(0);
  }

  const auto is_vr = configuration_table->get_as<bool>("is_vr").value_or(false);
  if(is_vr)
  {
    std::string system_name = configuration_table->get_as<std::string>("vr_system").value_or("none");
    if(system_name != "none")
    {
#if USING_OPENVR
      if(system_name == "openvr") { vr = std::make_unique<vr_system_openvr>(); }
#endif
#if USING_OPENXR
      if(system_name == "openxr") { vr = std::make_unique<vr_system_openxr>(); }
#endif
#if USING_OCULUS_VR
      if(system_name == "oculus") { vr = std::make_unique<vr_system_oculus>(); }
#endif

      if(!vr)
      {
        std::cerr << "application is VR, but VR system named " << system_name
                  << " is unknown to the framework. Check build options, and check config.toml for correctness\n";
      }
    }
  }

  if(vr) { mr_activated = configuration_table->get_as<bool>("use_LIV").value_or(false); }

  //create window
  window = sdl::Window(application_name,
                       window_size,
                       SDL_WINDOW_OPENGL | (hidpi ? SDL_WINDOW_ALLOW_HIGHDPI : 0)
                           | (fullscreen ? SDL_WINDOW_FULLSCREEN : SDL_WINDOW_RESIZABLE));
}

void application::create_opengl_context()
{
  //create OpenGL context
  context = window.create_context();
  context.make_current();
  std::cout << "OpenGL " << glGetString(GL_VERSION) << '\n';
  const auto bit_depth = sdl::Window::gl_get_attribute(SDL_GL_DEPTH_SIZE);
  std::cout << "Depth buffer is " << bit_depth << " bits wide\n";

  glEnable(GL_MULTISAMPLE);
}

scene* application::get_main_scene() { return main_scene; }

void application::initialize_modern_opengl() const
{
  initialize_glew();
  install_opengl_debug_callback();
}

void application::initialize_gui()
{
  ui = gui(window.ptr(), context.ptr());
  scripts.register_imgui_library(&ui);
  ui.set_script_engine_ptr(&scripts);
  ui.set_console_input_consumer(&scripts);
  inputs.setup_imgui();
}

void application::frame_prepare()
{
  const auto opengl_debug_tag = opengl_debug_group("application::frame_prepare()");
  (void)opengl_debug_tag;

  sun.direction               = glm::normalize(sun_direction_unormalized);
  s.scene_root->update_world_matrix();
  //The camera world matrix is stored inside the camera to permit to compute the camera view matrix
  main_camera->set_world_matrix(cam_node->get_world_matrix());
  shader_program_manager::set_frame_uniform(shader::uniform::gamma, shader::gamma);
  shader_program_manager::set_frame_uniform(shader::uniform::camera_position, cam_node->local_xform.get_position());
  shader_program_manager::set_frame_uniform(shader::uniform::view, main_camera->get_view_matrix());
  shader_program_manager::set_frame_uniform(shader::uniform::projection, main_camera->get_projection_matrix());
  shader_program_manager::set_frame_uniform(shader::uniform::main_directional_light, sun);
  shader_program_manager::set_frame_uniform(shader::uniform::point_light_0, *p_lights[0]);
  shader_program_manager::set_frame_uniform(shader::uniform::point_light_1, *p_lights[1]);
  shader_program_manager::set_frame_uniform(shader::uniform::point_light_2, *p_lights[2]);
  shader_program_manager::set_frame_uniform(shader::uniform::point_light_3, *p_lights[3]);
  shader_program_manager::set_frame_uniform(shader::uniform::light_space_matrix, light_space_matrix);

  glActiveTexture(GL_TEXTURE0 + shader::material_shadow_map_texture_slot);
  glBindTexture(GL_TEXTURE_2D, shadow_depth_map);
  shader_program_manager::set_frame_uniform(shader::uniform::shadow_map, shader::material_shadow_map_texture_slot);

  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  const auto size = window.size();
  main_camera->update_projection(size.x, size.y);
}

void application::render_shadowmap()
{
  const auto opengl_debug_tag = opengl_debug_group("application::render_shadowmap()");
  (void)opengl_debug_tag;

  sun.direction               = glm::normalize(sun_direction_unormalized);

  //Set the opengl to be the full shadowmap
  glViewport(0, 0, shadow_width, shadow_height);
  glBindFramebuffer(GL_FRAMEBUFFER, shadow_depth_fbo);
  glClear(GL_DEPTH_BUFFER_BIT);

  auto& shader = shader_program_manager::get_from_handle(shadowmap_shader);
  shader.use();
  //set the matrices and everything

  //calculate an orthographic projection for the directional light
  glm::mat4 light_projection = glm::ortho(-shadow_map_ortho_scale,
                                          shadow_map_ortho_scale,
                                          -shadow_map_ortho_scale,
                                          shadow_map_ortho_scale,
                                          shadow_map_near_plane,
                                          shadow_map_far_plane);
  glm::mat4 light_view       = glm::lookAt(-(shadow_map_direction_multiplier * sun.direction), glm::vec3(0.f), transform::Y_AXIS);
  light_space_matrix = light_projection * light_view;

  //glDisable(GL_CULL_FACE);
  //glCullFace(GL_BACK);

  //draw everything...
  s.scene_root->update_world_matrix(); //This compute the whole model matrices
  s.run_on_whole_graph([&](node* current_node) {
    current_node->visit([&](auto&& node_attached_object) {
      using T = std::decay_t<decltype(node_attached_object)>;
      if constexpr(std::is_same_v<T, scene_object>)
      {
        auto& object = static_cast<scene_object&>(node_attached_object);
        for(const auto submesh : object.get_mesh().get_submeshes())
        {
          auto& to_render = renderable_manager::get_from_handle(submesh);
          shader.set_uniform(shader::uniform::model, current_node->get_world_matrix());
          shader.set_uniform(shader::uniform::light_space_matrix, light_space_matrix);
          to_render.submit_draw_call();
        }
      }
    });
  });
 // glCullFace(GL_FRONT);
  //glEnable(GL_CULL_FACE);
  glFinish();

  //ImGui::Image(ImTextureID(size_t(shadow_depth_map)), ImVec2(256, 256), ImVec2(0, 1), ImVec2(1, 0));


  glBindFramebuffer(GL_FRAMEBUFFER, 0);
  const auto size = window.size();
  glViewport(0, 0, size.x, size.y);
}

void application::render_draw_list(camera* render_camera)
{
  const auto opengl_debug_tag = opengl_debug_group("application::render_draw_list");
  (void)opengl_debug_tag;
  for(auto [node, handle] : draw_list)
  {
    auto& to_draw = renderable_manager::get_from_handle(handle);
    to_draw.set_model_matrix(node->get_world_matrix());
    to_draw.set_mvp_matrix(render_camera->get_view_projection_matrix() * node->get_world_matrix());
    to_draw.draw();
  }
}

void application::build_draw_list_from_camera(camera* render_camera)
{
  const auto opengl_debug_tag = opengl_debug_group("application::build_draw_list_from_camera()");
  (void)opengl_debug_tag;

  draw_list.clear();

  s.run_on_whole_graph([&](node* current_node) {
    current_node->visit([&](auto&& node_attached_object) {
      //Only run this code if object is a scene_object
      using T = std::decay_t<decltype(node_attached_object)>;
      if constexpr(std::is_same_v<T, scene_object>)
      {
        auto& object                  = static_cast<scene_object&>(node_attached_object);
        const auto obb_points_list    = object.get_obb(current_node->get_world_matrix());
        const glm::mat4 to_clip_space = render_camera->get_view_projection_matrix();
        std::array<glm::vec4, 8> clip_space_obb { glm::vec4(0) };
        for(size_t j = 0; j < obb_points_list.size(); ++j)
        {
          const auto& obb_points = obb_points_list[j];
          //project the oriented bounding box to clip space
          for(size_t i = 0; i < 8; ++i) clip_space_obb[i] = to_clip_space * glm::vec4(obb_points[i], 1.f);

          //Start assuming object is visible
          bool outside = false;
          for(glm::vec4::length_type direction = 0; direction < 3; direction++) //testing 2 planes at the same time
          {
            const bool outside_positive_plane = (clip_space_obb[0][direction] > clip_space_obb[0].w)
                && (clip_space_obb[1][direction] > clip_space_obb[1].w) && (clip_space_obb[2][direction] > clip_space_obb[2].w)
                && (clip_space_obb[3][direction] > clip_space_obb[3].w) && (clip_space_obb[4][direction] > clip_space_obb[4].w)
                && (clip_space_obb[5][direction] > clip_space_obb[5].w) && (clip_space_obb[6][direction] > clip_space_obb[6].w)
                && (clip_space_obb[7][direction] > clip_space_obb[7].w);

            const bool outside_negative_plane = (clip_space_obb[0][direction] < -clip_space_obb[0].w)
                && (clip_space_obb[1][direction] < -clip_space_obb[1].w) && (clip_space_obb[2][direction] < -clip_space_obb[2].w)
                && (clip_space_obb[3][direction] < -clip_space_obb[3].w) && (clip_space_obb[4][direction] < -clip_space_obb[4].w)
                && (clip_space_obb[5][direction] < -clip_space_obb[5].w) && (clip_space_obb[6][direction] < -clip_space_obb[6].w)
                && (clip_space_obb[7][direction] < -clip_space_obb[7].w);

            outside = outside || outside_positive_plane || outside_negative_plane;
          }

          if(!outside) draw_list.push_back({ current_node, object.get_mesh().get_submeshes()[j] });

          //independently of the frustum culling, draw the bouding box
          if(debug_draw_bbox)
          {
            auto scene_obj                   = static_cast<scene_object>(node_attached_object);
            const auto opengl_debug_tag_obbs = opengl_debug_group("debug_draw_bbox");
            (void)opengl_debug_tag_obbs;

            GLint bind_back;
            glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &bind_back);
            glBindVertexArray(bbox_drawer_vao);
            glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(float), obb_points.data(), GL_STREAM_DRAW);
            const auto& debug_shader_object = shader_program_manager::get_from_handle(color_debug_shader);
            debug_shader_object.use();
            debug_shader_object.set_uniform(shader::uniform::view, render_camera->get_view_matrix());
            debug_shader_object.set_uniform(shader::uniform::projection, render_camera->get_projection_matrix());
            debug_shader_object.set_uniform(shader::uniform::debug_color, glm::vec4(1, 1, 1, 1));
            glDrawElements(GL_LINES, 24, GL_UNSIGNED_SHORT, nullptr);
            debug_shader_object.set_uniform(shader::uniform::debug_color, glm::vec4(0.4, 0.4, 1, 1));
            glDrawArrays(GL_POINTS, 0, 8);
            glBindVertexArray(bind_back);
          }
        }
      }
    });
  });
}

void application::render_skybox(camera* skybox_camera) {
  opengl_debug_group group("application::render_skybox");
  (void)(group);

  const auto projection = skybox_camera->get_projection_matrix();
  const auto view       = glm::mat4(glm::mat3(skybox_camera->get_view_matrix()));

  glDepthMask(GL_FALSE);
  glBindVertexArray(skybox_vao);
  glActiveTexture(GL_TEXTURE0 + shader::skybox_texture_slot);
  glBindTexture(GL_TEXTURE_CUBE_MAP, skybox->get_texture());
  auto& shader = shader_manager.get_from_handle(skybox_shader);
  shader.use();
  shader.set_uniform(shader::uniform::projection, projection);
  shader.set_uniform(shader::uniform::view, view);
  shader.set_uniform(shader::uniform::cubemap, shader::skybox_texture_slot);
  glDrawArrays(GL_TRIANGLES, 0, 36);
  glDepthMask(GL_TRUE);
}

void application::render_frame()
{
//  sun_direction_unormalized.z = glm::sin(current_time_in_sec * 0.5);


  //When using VR, the VR system is the master of the framerate!
  if(vr)
  {
    vr->wait_until_next_frame();
    vr->update_tracking();
  }

  //gameplay side thingies
  ui.frame();
  //scripts.update(last_frame_delta_sec);

  frame_prepare();
  render_shadowmap();
  render_skybox(main_camera);

  if(vr)
  {
    {
      opengl_debug_group group("VR rendering");
      //We invert the culling position if the vr system returns true to must_vflip.
      //This is because this system will do a vertical flip in the projection matrix.
      if(vr->must_vflip()) glFrontFace(GL_CCW);

      const sdl::Vec2i left_size  = vr->get_eye_framebuffer_size(vr_system::eye::left),
                       right_size = vr->get_eye_framebuffer_size(vr_system::eye::right);

      glBindFramebuffer(GL_FRAMEBUFFER, vr->get_eye_framebuffer(vr_system::eye::left));
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      camera* left_eye = vr->get_eye_camera(vr_system::eye::left);
      left_eye->update_projection(left_size.x, left_size.y);
      render_skybox(left_eye);
      build_draw_list_from_camera(left_eye);
      render_draw_list(left_eye);

      glBindFramebuffer(GL_FRAMEBUFFER, vr->get_eye_framebuffer(vr_system::eye::right));
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
      camera* right_eye = vr->get_eye_camera(vr_system::eye::right);
      right_eye->update_projection(right_size.x, right_size.y);
      render_skybox(right_eye);
      build_draw_list_from_camera(right_eye);
      render_draw_list(right_eye);

      vr->submit_frame_to_vr_system();
      if(vr->must_vflip()) glFrontFace(GL_CW);
    }

#ifdef _WIN32 //LIV.app only exist on Windows
    if(mr_activated)
    {
      //In case we haven't initialized Mixed Reality : This poke the SharedTextureProtocol to know if LIV is capturing
      if(!vr->is_mr_active()) vr->try_start_mr();

      if(vr->is_mr_active())
      {
        const opengl_debug_group group("MR rendering");

        //Updade mixed reality tracking - Will only work if vr is a `vr_system_openvr`
        vr->update_mr_camera();

        //Configure camera to draw on correct framebuffer object
        glBindFramebuffer(GL_FRAMEBUFFER, vr->get_mr_fbo());
        auto* mr_camera             = vr->get_mr_camera();
        const auto mr_viewport_size = vr->get_mr_size();
        build_draw_list_from_camera(mr_camera); //frustum culling

        //Clear the MR texture with transparent black
        const auto saved_clear_color = clear_color;
        set_clear_color(glm::vec4(0, 0, 0, 0));
        glClear(GL_COLOR_BUFFER_BIT);
        set_clear_color(saved_clear_color);

        //Render "foreground" pass to the bottom half of render target
        mr_camera->update_projection(mr_viewport_size.x, mr_viewport_size.y / 2, 0, mr_viewport_size.y / 2);
        vr->mr_depth_buffer_clear();
        vr->depth_buffer_write_depth_plane(); //This fill the depth buffer so only object in front of user are drawn
        render_draw_list(mr_camera);

        //Render everything to the background pass
        mr_camera->update_projection(mr_viewport_size.x, mr_viewport_size.y / 2);
        vr->mr_depth_buffer_clear();
        render_skybox(mr_camera);
        render_draw_list(mr_camera);

        vr->submit_to_LIV(); //This causes resource copy from GL to DX
      }
    }
#endif

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }

  const auto size = window.gl_get_drawable_size();
  main_camera->update_projection(size.x, size.y);
  build_draw_list_from_camera(main_camera);
  render_draw_list(main_camera);

  draw_debug_ui();
  ui.render();

  if(debug_draw_physics)
    physics.draw_phy_debug(
        main_camera->get_view_matrix(), main_camera->get_projection_matrix(), line_drawer_vao, vertex_debug_shader);

  //swap buffers
  window.gl_swap();


  frames++;
}

void application::run_events()
{
  //event polling
  while(event.poll())
  {
    //Check windowing events
    if(event.type == SDL_QUIT) running = false;

    //For ImGui
    ui.handle_event(event);

    //Process input events
    if(const auto command { inputs.process_input_event(event) }; command) command->execute();
  }

  fps_camera_controller->apply_movement(last_frame_delta_sec);
}

void application::run_script_update()
{
  s.run_on_whole_graph([](node* current_node) {
    if(auto* script = current_node->get_script_interface(); script) script->update();
  });
}

void application::run()
{
  while(running)
  {
#ifdef _DEBUG
#ifdef USING_JETLIVE
    liveInstance.update();
#endif
#endif
    update_timing();
    run_physics();
    run_events();
    run_script_update();
    render_frame();
  }
}

void application::setup_lights()
{
  std::array<node*, 4> lights { nullptr, nullptr, nullptr, nullptr };

  lights[0] = s.scene_root->push_child(create_node("light_0"));
  lights[1] = s.scene_root->push_child(create_node("light_1"));
  lights[2] = s.scene_root->push_child(create_node("light_2"));
  lights[3] = s.scene_root->push_child(create_node("light_3"));

  for(size_t i = 0; i < 4; ++i)
  {
    auto* l     = lights[i];
    auto* pl    = l->assign(point_light());
    pl->ambient = glm::vec3(0.1f);
    pl->diffuse = pl->specular = glm::vec3(0.9f, 0.85f, 0.8f) * 1.0f / 4.0f;
    p_lights[i]                = (pl);
  }

  lights[0]->local_xform.set_position(glm::vec3(-4.f, 3.f, -4.f));
  lights[1]->local_xform.set_position(glm::vec3(-4.f, -3.f, -4.f));
  lights[2]->local_xform.set_position(glm::vec3(-1.5f, 3.f, 1.75f));
  lights[3]->local_xform.set_position(glm::vec3(-1.f, 0.75f, 1.75f));

  sun.diffuse = sun.specular = glm::vec3(1);
  sun.specular *= 42;
  sun.ambient   = glm::vec3(0);
  sun.direction = glm::normalize(sun_direction_unormalized);
}

void application::setup_scene()
{
  //TODO everything about this should be done depending on some input somewhere, or provided by the game code - also, a level system would be useful
  main_scene = &s;

  if(vr)
  {
    vr->set_anchor(s.scene_root->push_child(create_node("vr_system_anchor")));
    vr->build_camera_node_system();
  }

  const texture_handle polutropon_logo_texture = texture_manager::create_texture();
  {
    const auto img                       = image("/polutropon.png");
    auto& polutropon_logo_texture_object = texture_manager::get_from_handle(polutropon_logo_texture);
    polutropon_logo_texture_object.load_from(img);
    polutropon_logo_texture_object.generate_mipmaps();
    polutropon_logo_texture_object.set_filtering_parameters();
  }

  // clang-format off
	const std::vector<float> plane =
	{
		//x=    y=     z=     uv=    normal=   tangent=
		-0.9f,  0.0f,  0.9f,  0, 1,  0, 1, 0,  0, -1, 0,
		 0.9f,  0.0f,  0.9f,  1, 1,  0, 1, 0,  0, -1, 0,
		-0.9f,  0.0f, -0.9f,  0, 0,  0, 1, 0,  0, -1, 0,
		 0.9f,  0.0f, -0.9f,  1, 0,  0, 1, 0,  0, -1, 0
	};

	const std::vector<unsigned int> plane_indices =
	{
		0, 1, 2, //triangle 0
		1, 3, 2  //triangle 1
	};
  // clang-format on

  const shader_handle simple_shader
      = shader_program_manager::create_shader("/shaders/simple.vert.glsl", "/shaders/simple.frag.glsl");
  const shader_handle unlit_shader
      = shader_program_manager::create_shader("/shaders/simple.vert.glsl", "/shaders/unlit.frag.glsl");

  const renderable_handle textured_plane_primitive
      = renderable_manager::create_renderable(simple_shader,
                                              plane,
                                              plane_indices,
                                              renderable::vertex_buffer_extrema { { -0.9f, 0.f, -0.9f }, { 0.9f, 0.f, 0.9f } },
                                              renderable::configuration { true, true, true, true },
                                              3 + 2 + 3 + 3,
                                              0,
                                              3,
                                              5,
                                              8);
  renderable_manager::get_from_handle(textured_plane_primitive).set_diffuse_texture(polutropon_logo_texture);
  mesh textured_plane;
  textured_plane.add_submesh(textured_plane_primitive);

  gltf = gltf_loader(simple_shader);

  cam_node = s.scene_root->push_child(create_node("application_camera"));
  {
    camera cam_obj;
    cam_obj.fov = 45;
    cam_node->local_xform.set_position({ 0, 1.75f, 5 });
    cam_node->local_xform.set_orientation(glm::angleAxis(glm::radians(-45.f), transform::X_AXIS));
    cam_node->assign(std::move(cam_obj));
    main_camera = cam_node->get_if_is<camera>();
    assert(main_camera);

    cam_node->push_child(create_node("audio_listener_position"))->assign(listener_marker());
  }
  fps_camera_controller = std::make_unique<camera_controller>(cam_node);

  inputs.register_keypress(SDL_SCANCODE_A, fps_camera_controller->press(camera_controller_command::movement_type::left));
  inputs.register_keypress(SDL_SCANCODE_D, fps_camera_controller->press(camera_controller_command::movement_type::right));
  inputs.register_keypress(SDL_SCANCODE_W, fps_camera_controller->press(camera_controller_command::movement_type::up));
  inputs.register_keypress(SDL_SCANCODE_S, fps_camera_controller->press(camera_controller_command::movement_type::down));
  inputs.register_keyrelease(SDL_SCANCODE_A, fps_camera_controller->release(camera_controller_command::movement_type::left));
  inputs.register_keyrelease(SDL_SCANCODE_D, fps_camera_controller->release(camera_controller_command::movement_type::right));
  inputs.register_keyrelease(SDL_SCANCODE_W, fps_camera_controller->release(camera_controller_command::movement_type::up));
  inputs.register_keyrelease(SDL_SCANCODE_S, fps_camera_controller->release(camera_controller_command::movement_type::down));
  inputs.register_mouse_motion_command(fps_camera_controller->mouse_motion());
  inputs.register_keyany(SDL_SCANCODE_LSHIFT, fps_camera_controller->run());
  inputs.register_gamepad_button_down_command(SDL_CONTROLLER_BUTTON_START, 0, &gamepad_button_test_command);

  setup_lights();

  std::cout << "Loading " << start_level_name << "as a level...";
  //TODO do not pass these objects here, initialize the level system with them:
  if(levels.load_level(scripts, gltf, s, start_level_name)) { std::cout << "Loading successful\n"; }
  else
    std::cout << "Something failed!";


  glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);

  audio.play_bgm("/bgm/bensound-slowmotion.wav");

  if(vr)
  {
    //Load some geometry and assign it to the hand nodes
    const auto vr_controller_mesh = gltf.load_mesh("gltf/vague_controller.glb"); //place holder red arrow thing
    const auto left_controller    = vr->load_controller_model_from_runtime(vr_controller::hand_side::left, unlit_shader);
    const auto right_controller   = vr->load_controller_model_from_runtime(vr_controller::hand_side::right, unlit_shader);

    bool has_left_controller  = false;
    bool has_right_controller = false;
    mesh left_controller_mesh;
    mesh right_controller_mesh;

    if(left_controller.renderable != renderable_manager::invalid_renderable)
    {
      auto& renderable = renderable_manager::get_from_handle(left_controller.renderable);
      renderable.set_diffuse_texture(left_controller.diffuse_texture != texture_manager::invalid_texture
                                         ? left_controller.diffuse_texture
                                         : texture_manager::get_dummy_texture());
      left_controller_mesh.add_submesh(left_controller.renderable);
      has_left_controller = true;
    }

    if(right_controller.renderable != renderable_manager::invalid_renderable)
    {
      auto& renderable = renderable_manager::get_from_handle(right_controller.renderable);
      renderable.set_diffuse_texture(right_controller.diffuse_texture != texture_manager::invalid_texture
                                         ? right_controller.diffuse_texture
                                         : texture_manager::get_dummy_texture());
      right_controller_mesh.add_submesh(right_controller.renderable);
      has_right_controller = true;
    }

    if(auto* left_hand = vr->get_hand(vr_controller::hand_side::left); left_hand)
    {
      auto node = left_hand->push_child(create_node("left_hand_tracker"));
      node->assign(scene_object((has_left_controller ? left_controller_mesh : vr_controller_mesh)));
    }
    if(auto* right_hand = vr->get_hand(vr_controller::hand_side::right); right_hand)
    {
      auto node = right_hand->push_child(create_node("right_hand_tracker"));
      node->assign(scene_object((has_right_controller ? right_controller_mesh : vr_controller_mesh)));
    }
  }

  //Create a scene node attached to the root (so local xform == world xform)
  //physics_test = s.scene_root->push_child(create_node("physics_test_node"));
  //physics_test->local_xform.set_position(glm::vec3(0, 2, 0));

  ////Load the 3D model of that fish from the Krhonos repo
  //const auto physics_test_geometry = gltf.load_mesh("/gltf/BarramundiFish.glb");
  //physics_test->assign(scene_object(physics_test_geometry));

  ////This 20cm box body will represent the fish
  //test_box = new physics_system::box_proxy(physics_test->local_xform.get_position(), glm::vec3(0.1f), 1);
  //physics.add_to_world(*test_box);
}

void application::run_physics()
{
  physics.step_simulation(last_frame_delta_sec);

  //Move the damn fish's node by hand so we can see that gravity works
  //physics_test->local_xform = test_box->get_world_transform();
}

void application::set_clear_color(glm::vec4 color)
{
  if(clear_color != color)
  {
    clear_color = color;
    glClearColor(clear_color.r, clear_color.g, clear_color.b, clear_color.a);
  }
}

void application::gamepad_button_test_command_::execute() { std::cout << "You pressed the button\n"; }

void application::splash_frame(const char* image_path)
{
  sdl::Event e;
  while(e.poll())
  {
    //empty the message queue
  }

  glClearColor(0, 0, 0, 0);
  glClear(GL_COLOR_BUFFER_BIT);

  try
  {
    const auto splash_txt_handle = texture_mgr.create_texture();
    auto& splash_txt             = texture_mgr.get_from_handle(splash_txt_handle);

    const auto default_splash = image(image_path);
    std::cerr << "default splash dimentions is " << default_splash.get_width() << "x" << default_splash.get_height() << "\n";
    splash_txt.load_from(default_splash, true, GL_TEXTURE_2D);
    splash_txt.generate_mipmaps(GL_TEXTURE_2D);

    auto splash_shader_handle = shader_program_manager::create_shader("shaders/splash.vert.glsl", "shaders/splash.frag.glsl");
    auto& splash_shader       = shader_program_manager::get_from_handle(splash_shader_handle);

    float vtx_obj[] = { -1, -1, 0, 0, 3, -1, 2, 0, -1, 3, 0, -2 };

    GLuint splash_buffer, splash_buffer_vao;
    glGenVertexArrays(1, &splash_buffer_vao);
    glGenBuffers(1, &splash_buffer);
    glBindVertexArray(splash_buffer_vao);
    glBindBuffer(GL_ARRAY_BUFFER, splash_buffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vtx_obj), vtx_obj, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    splash_txt.bind(0, GL_TEXTURE_2D);
    splash_shader.use();
    const auto raw_program = splash_shader._get_program();
    glUniform1i(glGetUniformLocation(raw_program, "splash"), 0);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
  }
  catch(const std::exception& e)
  {
    std::cerr << "did not load shader for splash! :" << e.what() << "\n";
  }

  window.gl_swap();
  glGetError();
  while(e.poll())
  {
    //empty the message queue
  }
}

application* application::instance = nullptr;
application::application(int argc, char** argv, const std::string& application_name) : resources(argc > 0 ? argv[0] : nullptr)
{

  if(instance != nullptr) throw std::runtime_error("Only one application can exist");
  instance = this;

  //Bind the debug utilities
  inputs.register_keypress(SDL_SCANCODE_GRAVE, &keyboard_debug_utilities.toggle_console_keyboard_command);
  inputs.register_keypress(SDL_SCANCODE_TAB, &keyboard_debug_utilities.toggle_debug_keyboard_command);
  inputs.register_keyrelease(SDL_SCANCODE_R, &keyboard_debug_utilities.toggle_live_code_reload_command);

  //Install resources
  for(const auto& pak : resource_paks)
  {
    std::cerr << "Adding to resources " << pak << '\n';
    resource_system::add_location(pak);
  }

  //Basic initialization of the rendering context
  configure_and_create_window(application_name);
  create_opengl_context();
  initialize_modern_opengl();
  texture_manager::initialize_dummy_texture();

  //We now have enough of a renderer available to display something on screen and pump some events
  //Some OSes (like macOS) really likes that we can do this now so they actually show a window and not apear to hang
  splash_frame();
  initialize_gui();

  //attempt init VR
  if(vr)
  {
    if(vr->initialize(window))
      vr->initialize_opengl_resources();
    else
      vr = nullptr;
  }

  //Initialize main shadow map
  try
  {
    shadowmap_shader = shader_program_manager::create_shader("/shaders/shadow.vert.glsl", "/shaders/shadow.frag.glsl");
    glGenFramebuffers(1, &shadow_depth_fbo);
    glGenTextures(1, &shadow_depth_map);
    glBindTexture(GL_TEXTURE_2D, shadow_depth_map);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadow_width, shadow_height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
    const GLfloat border_color[] { 1.0, 1.0, 1.0, 1.0 };
    glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, border_color);
    glBindFramebuffer(GL_FRAMEBUFFER, shadow_depth_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadow_depth_map, 0);
    glDrawBuffer(GL_NONE);
    glReadBuffer(GL_NONE);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
  }
  catch(const std::exception& e)
  {
    sdl::show_message_box(SDL_MESSAGEBOX_ERROR, "Could not create shadowmap shader!", e.what());
    throw;
  }

  //Initialize skybox
  try
  {
    skybox_shader = shader_program_manager::create_shader("/shaders/skybox.vert.glsl", "/shaders/skybox.frag.glsl");

    std::array<image, 6> skybox_images { { { "/skybox/right.jpg" },
                                           { "/skybox/left.jpg" },
                                           { "/skybox/top.jpg" },
                                           { "/skybox/bottom.jpg" },
                                           { "/skybox/front.jpg" },
                                           { "/skybox/back.jpg" } } };
    skybox        = std::make_unique<cubemap>(skybox_images);
    const float skybox_vertices[] //This is just the 36 vertices of a cube drawn with tris
        = {
            -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f,
            1.0f,  -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,
            1.0f,  -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  1.0f,  1.0f,  1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, 1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,
            1.0f,  1.0f,  1.0f,  1.0f,  -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,
            1.0f,  -1.0f, -1.0f, 1.0f,  -1.0f, -1.0f, -1.0f, -1.0f, 1.0f,  1.0f,  -1.0f, 1.0f
          };

    glGenVertexArrays(1, &skybox_vao);
    glBindVertexArray(skybox_vao);
    glGenBuffers(1, &skybox_vbo);
    glBindBuffer(GL_ARRAY_BUFFER, skybox_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(skybox_vertices), skybox_vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glBindVertexArray(0);
  }
  catch(const std::exception& e)
  {
    sdl::show_message_box(SDL_MESSAGEBOX_ERROR, "Could not create skybox shader!", e.what());
    throw;
  }

  setup_scene();

  sdl::Mouse::set_relative(true);

  //shader that draws everything in white
  try
  {
    color_debug_shader = shader_program_manager::create_shader("/shaders/debug.vert.glsl", "/shaders/debug.frag.glsl");
    glGenBuffers(1, &bbox_drawer_vbo);
    glGenBuffers(1, &bbox_drawer_ebo);
    glGenVertexArrays(1, &bbox_drawer_vao);
    glBindVertexArray(bbox_drawer_vao);
    glBindBuffer(GL_ARRAY_BUFFER, bbox_drawer_vbo);
    float empty[8 * 3] { 0 };
    glBufferData(GL_ARRAY_BUFFER, 8 * 3 * sizeof(float), empty, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), static_cast<void*>(0));
    glEnableVertexAttribArray(0);
    const unsigned short int lines[] {
      0, 1, 1, 2, 2, 3, 3, 0, 4, 5, 5, 6, 6, 7, 7, 4, 0, 4, 1, 5, 2, 6, 3, 7
    }; //see renderable.hpp about renderable bounds to check what these indices are
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, bbox_drawer_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(lines), lines, GL_STATIC_DRAW);
    glPointSize(10);
    glLineWidth(5);
    glBindVertexArray(0);

    glGenBuffers(1, &line_drawer_vbo);
    glGenVertexArrays(1, &line_drawer_vao);
    glBindVertexArray(line_drawer_vao);
    glBindBuffer(GL_ARRAY_BUFFER, line_drawer_vbo);
    float empty_line[2 * 3] {0};
    glBufferData(GL_ARRAY_BUFFER, 2 * 3 * sizeof(float), empty_line, GL_STREAM_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 7 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 7 * sizeof(float), (void*)( 3 * sizeof(float)));
    glEnableVertexAttribArray(1);
  }
  catch(const std::exception& e)
  {
    sdl::show_message_box(SDL_MESSAGEBOX_WARNING, "Could not initialize the debug shader", e.what());
    color_debug_shader = shader_program_manager::invalid_shader;
  }

  try
  {
    vertex_debug_shader = shader_program_manager::create_shader("/shaders/debug_vertex_color.vert.glsl", "/shaders/debug_vertex_color.frag.glsl");

  }
  catch(const std::exception& e)
  {
    sdl::show_message_box(SDL_MESSAGEBOX_WARNING, "Could not initialize the debug shader", e.what());
    vertex_debug_shader = shader_program_manager::invalid_shader;

  }

  glEnable(GL_DEPTH_TEST);
  glEnable(GL_CULL_FACE);
  glCullFace(GL_FRONT);
  glFrontFace(GL_CW);

  scripts.evaluate_file("/scripts/test.chai");

  physics.set_gravity(physics_system::earth_average_gravitational_acceleration_field);
  physics.add_ground_plane();
}

void application::keyboard_debug_utilities_::toggle_console_keyboard_command_::execute()
{
  if(parent_->ui.is_console_showed())
  {
    parent_->ui.hide_console();
    sdl::Mouse::set_relative(true);
  }
  else
  {
    parent_->ui.show_console();
    sdl::Mouse::set_relative(false);
  }
}

void application::keyboard_debug_utilities_::toggle_debug_keyboard_command_::execute()
{
  if(parent_->debug_ui)
  {
    sdl::Mouse::set_relative(true);
    parent_->debug_ui = false;
  }
  else
  {
    parent_->debug_ui = true;
  }
}

void application::keyboard_debug_utilities_::toggle_live_code_reload_command_::execute()
{
#ifdef _DEBUG
  if(modifier & KMOD_LCTRL)
  {
#ifdef USING_JETLIVE
    parent_->liveInstance.tryReload();
#else
    static bool first_print = false;
    if(!first_print) parent_->ui.push_to_console("If you are using blink, live reload is automatic.");
    first_print = true;
#endif
  }
#endif
}
