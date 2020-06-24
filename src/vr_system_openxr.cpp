#include "build_config.hpp"
#if USING_OPENXR

#include "vr_system_openxr.hpp"
#include "nameof.hpp"

#define max_properties_count 64
#include "openxr/openxr_platform.h"

#include "imgui.h"

//That's a bit ugly I know
XrView *left_eye_view = nullptr, *right_eye_view = nullptr;
bool vr_system_openxr::need_to_vflip = false;

vr_system_openxr::vr_system_openxr() : vr_system() { std::cout << "Initialized OpenXR based vr_system implementation\n"; }

vr_system_openxr::~vr_system_openxr()
{
  std::cout << "Deinitialized OpenXR based vr_system implementation\n";

  if(application_space != XR_NULL_HANDLE) { xrDestroySpace(application_space); }
  if(session != XR_NULL_HANDLE)
  {
    xrEndSession(session);
    xrDestroySession(session);
  }
  if(instance != XR_NULL_HANDLE) { xrDestroyInstance(instance); }
}

template <typename T>
inline void zero_it(T& obj)
{
  memset(reinterpret_cast<void*>(&obj), 0, sizeof(T));
}

template <typename T>
inline void zero_it(T obj[], size_t count)
{

  memset(reinterpret_cast<void*>(obj), 0, sizeof(T) * count);
}

bool vr_system_openxr::initialize(sdl::Window& window)
{
  std::cout << "Initializing OpenXR based VR system\n";
  const char* xr_runtime_json_str = getenv("XR_RUNTIME_JSON");
  if(xr_runtime_json_str != nullptr) std::cout << xr_runtime_json_str << "\n";

  //Step one, get XrInstance up and running

  //Enumerate layers and extensions
  XrApiLayerProperties api_layer_properties[max_properties_count];
  zero_it(api_layer_properties, max_properties_count);
  api_layer_properties->type = XR_TYPE_API_LAYER_PROPERTIES;
  uint32_t api_layer_count   = 0;
  XrResult status;

  if(status = xrEnumerateApiLayerProperties(max_properties_count, &api_layer_count, api_layer_properties); status != XR_SUCCESS)
  { std::cerr << "error while enumerating API properties\n"; }

  std::vector<const char*> api_layer_names((size_t)api_layer_count);
  for(size_t i = 0; i < api_layer_count; ++i) api_layer_names[i] = api_layer_properties[i].layerName;

  XrExtensionProperties extension_properties[max_properties_count];
  zero_it(extension_properties, max_properties_count);
  uint32_t extension_properties_count = 0;

  if(api_layer_count > 0) //So, it seems that if there's no layers to use, there's not point getting extension properties?
  {

    //the nullptr here is actually the layer name we want to have, but it *can* be null according to documentation
    if(status
       = xrEnumerateInstanceExtensionProperties(nullptr, max_properties_count, &extension_properties_count, extension_properties);
       status != XR_SUCCESS)
    { std::cerr << "error while enumerating instance extension properties\n"; }
  }
  std::vector<const char*> extension_properties_names((size_t)extension_properties_count);
  for(size_t i = 0; i < extension_properties_count; i++) extension_properties_names[i] = extension_properties[i].extensionName;

  //Last extension in the list will be the Graphics API to use: OpenGL
  extension_properties_names.push_back("XR_KHR_opengl_enable");
  extension_properties_count++;

  //Create OpenXR instance
  const char engine_name[] = "The //TODO engine";
  XrInstanceCreateInfo instance_create_info;
  zero_it(instance_create_info);
  instance_create_info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
  strcpy(instance_create_info.applicationInfo.engineName, engine_name);
  strcpy(instance_create_info.applicationInfo.applicationName, GAME_NAME);
  instance_create_info.type                  = XR_TYPE_INSTANCE_CREATE_INFO;
  instance_create_info.enabledApiLayerCount  = api_layer_count;
  instance_create_info.enabledApiLayerNames  = api_layer_names.data();
  instance_create_info.enabledExtensionCount = extension_properties_count;
  instance_create_info.enabledExtensionNames = extension_properties_names.data();

  if(status = xrCreateInstance(&instance_create_info, &instance); status != XR_SUCCESS)
  {
    std::cerr << "error while creating XrInstance\n";

#ifdef _WIN32
    /*
     * On the Windows platform, many of the early implementations of OpenXR are not available with
     * OpenGL. The common denominator between those seems to be DirectX 11. Thus, we added a special
     * system that permit to get the rendered images from the engine *into* DirectX11 from OpenGL.
     *
     * See the `gl_dx_interop` class for more info.
     */

    std::cerr << "attempt directx interop fallback...\n";
    extension_properties_names.back() = "XR_KHR_D3D11_enable"; //We change the name of the last extension and we try again

    //This will both, create a DirectX 11 device, device_contex and swapchain (owned by a dummy, hidden Window) *and* will check for the necessary WGL extensions
    dx11_interop = gl_dx11_interop::get();
    if(dx11_interop->init()) 
    {
      if(auto dx_status = xrCreateInstance(&instance_create_info, &instance); dx_status == XR_SUCCESS)
      {
        fallback_to_dx = true;
        need_to_vflip  = true;
      }
      else
      {
        return false; //we cannot recuperate from this
      }
    }
    else
    {
      return false;
    }
#else
    return false;
#endif
  }
  std::cout << "xrCreateInstance() == XR_SUCCESS\n";

  //Get properties of instance
  XrInstanceProperties instance_properties;
  zero_it(instance_properties);
  instance_properties.type = XR_TYPE_INSTANCE_PROPERTIES;
  if(status = xrGetInstanceProperties(instance, &instance_properties); status != XR_SUCCESS)
  { std::cerr << "error while getting the instance properties\n"; }
  else
  {
    std::cout << "OpenXR Runtime name    : " << instance_properties.runtimeName << "\n";
    std::cout << "OpenXR Runtime version : " << XR_VERSION_MAJOR(instance_properties.runtimeVersion) << "."
              << XR_VERSION_MINOR(instance_properties.runtimeVersion) << "."
              << XR_VERSION_PATCH(instance_properties.runtimeVersion) << "\n";
  }

  //Step two: get an XrSystem up and running

  //Get a HMD-style system
  XrSystemGetInfo system_get_info;
  zero_it(system_get_info);
  system_get_info.type       = XR_TYPE_SYSTEM_GET_INFO;
  system_get_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
  if(status = xrGetSystem(instance, &system_get_info, &system_id); status != XR_SUCCESS)
  { std::cerr << "error while getting system info for HMD form factor\n"; }

  //Retrieve system properties
  XrSystemProperties system_properties;
  zero_it(system_properties);
  system_properties.type = XR_TYPE_SYSTEM_PROPERTIES;
  if(status = xrGetSystemProperties(instance, system_id, &system_properties); status != XR_SUCCESS)
  { std::cerr << "error while getting system properties\n"; }

  std::cout << "HMD FormFactor system  : " << system_properties.systemName << "\n";
  std::cout << "Positional Tracking    : " << (system_properties.trackingProperties.positionTracking == XR_TRUE ? "YES" : "NO")
            << "\n";
  std::cout << "Rotational Tracking    : " << (system_properties.trackingProperties.orientationTracking == XR_TRUE ? "YES" : "NO")
            << "\n";
  std::cout << "Max Swapchain Size     : " << system_properties.graphicsProperties.maxSwapchainImageWidth << "x"
            << system_properties.graphicsProperties.maxSwapchainImageWidth << "\n";
  std::cout << "Max layer count        : " << system_properties.graphicsProperties.maxLayerCount << "\n";

  ///Step 3 : Get information about the Views we are going to render

  //Retrieve view configuration
  XrViewConfigurationType view_configuration_type[4];
  zero_it(view_configuration_type, 4);
  uint32_t view_configuration_type_count = 0;
  status = xrEnumerateViewConfigurations(instance, system_id, 4, &view_configuration_type_count, view_configuration_type);

  XrViewConfigurationType best_view_config_type = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
  if(view_configuration_type_count > 0) best_view_config_type = view_configuration_type[0];
  std::cout << "View configuration type : " << NAMEOF_ENUM(best_view_config_type) << "\n";
  //We are rendering a stereoscopic view in an HMD
  assert(best_view_config_type == XR_VIEW_CONFIGURATION_TYPE_PRIMARY_STEREO);
  used_view_configuration_type = best_view_config_type;

  XrViewConfigurationProperties view_configuration_properties;
  zero_it(view_configuration_properties);
  view_configuration_properties.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
  if(status = xrGetViewConfigurationProperties(instance, system_id, best_view_config_type, &view_configuration_properties);
     status != XR_SUCCESS)
  { std::cerr << "error: cannot get view configuration properties\n"; }
  else
  {
    //This is a weird feature, but like, well, we have a callback to recompute the projection matrix on the fly too so, it's not a problem
    std::cout << "Mutable FoV : " << (view_configuration_properties.fovMutable == XR_TRUE ? "YES" : "NO") << "\n";
  }

  XrViewConfigurationView view_configuration_view[4];
  zero_it(view_configuration_view, 4);
  for(auto& vcv : view_configuration_view) vcv.type = XR_TYPE_VIEW_CONFIGURATION_VIEW;
  uint32_t view_configuration_view_count = 0;
  if(status = xrEnumerateViewConfigurationViews(
         instance, system_id, best_view_config_type, 4, &view_configuration_view_count, view_configuration_view);
     status != XR_SUCCESS)
  { std::cerr << "error: cannot enumerate view configuration views\n"; }
  views.resize(view_configuration_view_count, { XR_TYPE_VIEW });
  left_eye_view  = &views[0];
  right_eye_view = &views[1];

  std::cout << "system has " << view_configuration_view_count << " " << NAMEOF_VAR_TYPE(view_configuration_view[0]) << "\n";
  for(size_t i = 0; i < view_configuration_view_count; ++i)
  {
    const auto& vcv = view_configuration_view[i];
    std::cout << "View index " << i << " :\n";
    std::cout << "\t - recommended image rect   : " << vcv.recommendedImageRectWidth << "x" << vcv.recommendedImageRectHeight
              << "\n";
    std::cout << "\t - maximum image rect       : " << vcv.maxImageRectWidth << "x" << vcv.maxImageRectHeight << "\n";
    std::cout << "\t - recommended sample count : " << vcv.recommendedSwapchainSampleCount << "\n";
    std::cout << "\t - maximum sample count     : " << vcv.maxSwapchainSampleCount << "\n";
  }

  //We only like... Support VR headset...
  assert(view_configuration_view_count >= 2);
  eye_render_target_sizes[0].x = view_configuration_view[0].recommendedImageRectWidth;
  eye_render_target_sizes[1].x = view_configuration_view[1].recommendedImageRectWidth;
  eye_render_target_sizes[0].y = view_configuration_view[0].recommendedImageRectHeight;
  eye_render_target_sizes[1].y = view_configuration_view[1].recommendedImageRectHeight;

  XrEnvironmentBlendMode environment_blend_mode[4];
  uint32_t environement_blend_mode_count = 0;
  if(status = xrEnumerateEnvironmentBlendModes(
         instance, system_id, best_view_config_type, 4, &environement_blend_mode_count, environment_blend_mode);
     status != XR_SUCCESS)
  { std::cerr << "error: cannot enumerate environement blend mode\n"; }

  assert(environement_blend_mode_count > 0);
  XrEnvironmentBlendMode best_environment_blend_mode = environment_blend_mode[0];
  std::cout << "Environement blend mode :" << NAMEOF_ENUM(best_environment_blend_mode) << "\n";

  //TODO need to learn about the **actions** API
  //XrActionSetCreateInfo action_set_create_info;
  //zero_it(action_set_create_info);
  //action_set_create_info.type = XR_TYPE_ACTION_SET_CREATE_INFO;
  //strcpy(action_set_create_info.actionSetName, "some_action_set");
  //strcpy(action_set_create_info.localizedActionSetName, action_set_create_info.actionSetName);
  //action_set_create_info.priority = -1;
  //XrActionSet action_set;
  //xrCreateActionSet(instance, &action_set_create_info, &action_set);
  //TODO xrSuggestInteractionProfileBindings()

  //Step 4 : Request graphcics backed requiredment for OpenGL

#ifdef _WIN32
  if(!fallback_to_dx)
  {
#endif
    XrGraphicsRequirementsOpenGLKHR graphics_requirements_opengl_khr;
    zero_it(graphics_requirements_opengl_khr);
    graphics_requirements_opengl_khr.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;

    //This function needs to be dynamically loaded because it's an extension?
    PFN_xrGetOpenGLGraphicsRequirementsKHR xrGetOpenGLGraphicsRequirementsKHR_dynamic;
    const auto has_func
        = xrGetInstanceProcAddr(instance,
                                "xrGetOpenGLGraphicsRequirementsKHR",
                                reinterpret_cast<PFN_xrVoidFunction*>(&xrGetOpenGLGraphicsRequirementsKHR_dynamic));
    if(has_func == XR_SUCCESS)
    {
      xrGetOpenGLGraphicsRequirementsKHR_dynamic(instance, system_id, &graphics_requirements_opengl_khr);
      //It seems not doing the above call cause a validation error
      std::cout << "OpenGL version min required : " << XR_VERSION_MAJOR(graphics_requirements_opengl_khr.minApiVersionSupported)
                << "." << XR_VERSION_MINOR(graphics_requirements_opengl_khr.minApiVersionSupported) << "\n";
    }
    else
    {
      std::cerr << "Cannot get pointer to xrGetOpenGLGraphicsRequirementsKHR\n";
    }
#ifdef _WIN32
  }
  else
  {
    XrGraphicsRequirementsD3D11KHR graphics_requirements_d3d11_khr;
    zero_it(graphics_requirements_d3d11_khr);
    graphics_requirements_d3d11_khr.type = XR_TYPE_GRAPHICS_REQUIREMENTS_D3D11_KHR;

    PFN_xrGetD3D11GraphicsRequirementsKHR xrGetD3D11GraphicsRequirementsKHR;
    const auto has_func = xrGetInstanceProcAddr(
        instance, "xrGetD3D11GraphicsRequirementsKHR", reinterpret_cast<PFN_xrVoidFunction*>(&xrGetD3D11GraphicsRequirementsKHR));
    if(has_func == XR_SUCCESS)
    {
      xrGetD3D11GraphicsRequirementsKHR(instance, system_id, &graphics_requirements_d3d11_khr);
      std::cout << "Minimal D3D11 feature level required " << NAMEOF_ENUM(graphics_requirements_d3d11_khr.minFeatureLevel)
                << "\n";
    }
    else
    {
      std::cerr << "Cannot get pointer to xrGetD3D11GraphicsRequirementsKHR\n";
    }
  }
#endif

  //Step 5 create session
  XrSessionCreateInfo session_create_info;
  zero_it(session_create_info);
  session_create_info.type     = XR_TYPE_SESSION_CREATE_INFO;
  session_create_info.systemId = system_id;

  //we need to get ghe graphics bindings that correspond to the choosen graphics API and windowing system
#ifdef WIN32
  XrGraphicsBindingOpenGLWin32KHR xr_graphics_binding_opengl;
  XrGraphicsBindingD3D11KHR xr_graphics_binding_d3d11;
  zero_it(xr_graphics_binding_opengl);
  xr_graphics_binding_opengl.type  = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
  xr_graphics_binding_opengl.hGLRC = wglGetCurrentContext();
  xr_graphics_binding_opengl.hDC   = wglGetCurrentDC();
  if(fallback_to_dx)
  {
    zero_it(xr_graphics_binding_d3d11);
    xr_graphics_binding_d3d11.type   = XR_TYPE_GRAPHICS_BINDING_D3D11_KHR;
    xr_graphics_binding_d3d11.device = dx11_interop->get_device();
  }
  session_create_info.next         = fallback_to_dx ? (void*)&xr_graphics_binding_d3d11 : (void*)&xr_graphics_binding_opengl;
#else //I want this to work on Linux sooo bad. But I cannot test it.
  XrGraphicsBindingOpenGLXlibKHR xr_graphics_binding;
  zero_it(xr_graphics_binding);
  //If needed:
  //SDL_SysWMinfo wm_info;
  //SDL_GetWindowWMInfo(window.ptr(), &wm_info);
  
  xr_graphics_binding.type = XR_TYPE_GRAPHICS_BINDING_OPENGL_XLIB_KHR;
  xr_graphics_binding.xDisplay = XOpenDisplay(NULL);
  xr_graphics_binding.glxContext = glXGetCurrentContext();
  xr_graphics_binding.glxDrawable = glXGetCurrentDrawable();
  //Note: the monado example do not set the FBCofnig and visualid on this structure

  session_create_info.next = &xr_graphics_binding;
#endif

  if(status = xrCreateSession(instance, &session_create_info, &session); status != XR_SUCCESS)
  { std::cerr << "error: cannot create session " << NAMEOF_ENUM(status) << "\n"; }
  else
  {
    std::cout << "XrCreateSession() == XR_SUCCESS\n";
  }

  //Step 6 define Reference space (for when we ask for tracking data
  XrPosef identity_pose;
  zero_it(identity_pose);
  identity_pose.orientation.w = 1.f;

  XrReferenceSpaceCreateInfo reference_space_create_info;
  zero_it(reference_space_create_info);
  reference_space_create_info.type                 = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
  reference_space_create_info.referenceSpaceType   = XR_REFERENCE_SPACE_TYPE_STAGE;
  reference_space_create_info.poseInReferenceSpace = identity_pose;
  zero_it(application_space);
  if(status = xrCreateReferenceSpace(session, &reference_space_create_info, &application_space); status != XR_SUCCESS)
  { std::cerr << NAMEOF_ENUM(status) << "\n"; }

  //Step 7 Create swapchains (list of images that are submitted to the compositor)
  int64_t format[32]; //32 is way too many options, there's like 10 max here, but well...
  zero_it(format, 32);
  uint32_t format_count = 0;
  xrEnumerateSwapchainFormats(session, 32, &format_count, format);

  //TODO switch between GL and D3D11
  //This is the format we want...
#ifdef _WIN32
  if(!fallback_to_dx)
  {
#endif
    if(std::find(std::begin(format), std::end(format), GL_RGBA8) != std::end(format))
    { std::cout << "found GL_RGBA8 in possible format list\n"; }
    else
    {
      std::cerr << "OpenGL texture format GL_RGBA8 not found within the supported formats by OpenXR runtime\n";
      //return false;
    }

    //...But actually we gamma corrected our rendering in shaders, so to avoid it being done twice over, we'll lie that our pixel format is this one:
    if(std::find(std::begin(format), std::end(format), GL_SRGB8_ALPHA8) != std::end(format))
    { std::cout << "found GL_SRGB8_ALPHA8 in possible format list\n"; }
    else
    {
      std::cerr << "OpenGL texture format GL_SRGB8_ALPHA8 not found within the supported formats by OpenXR runtime\n";
      //return false;
    }

    for(int i = 0; i < 32; ++i)
    {
      std::cout << "fomrat " << i << ": " << format[i] << std::endl;
    }

#ifdef _WIN32
  }
  else
  {
    if(std::find(std::begin(format), std::end(format), DXGI_FORMAT_R8G8B8A8_UNORM_SRGB) != std::end(format))
    { std::cout << "found DXGI_FORMAT_R8G8B8A8_UNORM_SRGB in possible format list\n"; }
    else
    {
      std::cerr
          << "DXGI Format DXGI_FORMAT_R8G8B8A8_UNORM_SRGB not found within the list of supported formats by OpenXR runtime\n";
      return false;
    }

    //TODO find our image format for some RGBA or BGRA or whatever we can manage to get here
  }
#endif

  //We are going to do stereo rendering, we want one swapchain per eye
  for(size_t i = 0; i < 2; ++i)
  {
    XrSwapchainCreateInfo swapchain_create_info;
    zero_it(swapchain_create_info);
    swapchain_create_info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
    swapchain_create_info.usageFlags
        = XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT; //Transfer Destination bit. We only ever transfer rendered images to this, we don't use it as part of the rendering.
    swapchain_create_info.mipCount    = 1;
    swapchain_create_info.sampleCount = 1;
    //See comments format enumeration above
    #ifdef _WIN32
    swapchain_create_info.format = fallback_to_dx
        ? (int64_t)DXGI_FORMAT_R8G8B8A8_UNORM_SRGB
        : (int64_t)
            GL_SRGB8_ALPHA8; //TODO check the spec about SRGB/Linear color spaces. The missmatch here is intentional, image's too bright without this
                             //GL_RGBA8;
    #else
    swapchain_create_info.format = GL_SRGB8_ALPHA8;
    #endif
    swapchain_create_info.faceCount = 1;
    swapchain_create_info.arraySize = 1;
    swapchain_create_info.width     = eye_render_target_sizes[i].x;
    swapchain_create_info.height    = eye_render_target_sizes[i].y;

    xrCreateSwapchain(session, &swapchain_create_info, &swapchain[i]);

    uint32_t swapchain_image_count = 0;
    XrSwapchainImageOpenGLKHR swapchain_image_opengl_khr[8];

#ifdef _WIN32
    XrSwapchainImageD3D11KHR swapchain_image_d3d11_khr[8];
    if(!fallback_to_dx)
    {
#endif
      zero_it(swapchain_image_opengl_khr, 8);
      for(auto& image_opengl : swapchain_image_opengl_khr) image_opengl.type = XR_TYPE_SWAPCHAIN_IMAGE_OPENGL_KHR;
      if(status = xrEnumerateSwapchainImages(
             swapchain[i], 4, &swapchain_image_count, (XrSwapchainImageBaseHeader*)swapchain_image_opengl_khr);
         status != XR_SUCCESS)
      { std::cerr << "error: could not get swapchain images for swapchain " << i << " " << NAMEOF_ENUM(status) << "\n"; }
      else
      {
        std::cout << "this swapchain has " << swapchain_image_count << " images.\n";
        for(size_t img_index = 0; img_index < swapchain_image_count; img_index++)
        { swapchain_images_opengl[i].push_back(swapchain_image_opengl_khr[img_index]); }
      }
#ifdef _WIN32
    }
    else
    {
      zero_it(swapchain_image_d3d11_khr, 8);
      for(auto& image_d3d11 : swapchain_image_d3d11_khr) image_d3d11.type = XR_TYPE_SWAPCHAIN_IMAGE_D3D11_KHR;
      if(status = xrEnumerateSwapchainImages(
             swapchain[i], 4, &swapchain_image_count, (XrSwapchainImageBaseHeader*)swapchain_image_d3d11_khr);
         status != XR_SUCCESS)
      { std::cerr << "error: could not get swapchain images for swapchain " << i << " " << NAMEOF_ENUM(status) << "\n"; }
      else
      {
        for(size_t img_index = 0; img_index < swapchain_image_count; img_index++)
        { swapchain_images_d3d11[i].push_back(swapchain_image_d3d11_khr[img_index]); }
      }
    }
#endif
  }

  //Step 8 "Begin" the session itself
  XrSessionBeginInfo xr_session_begin_info;
  zero_it(xr_session_begin_info);
  xr_session_begin_info.type                         = XR_TYPE_SESSION_BEGIN_INFO;
  xr_session_begin_info.primaryViewConfigurationType = best_view_config_type;
  if(status = xrBeginSession(session, &xr_session_begin_info); status != XR_SUCCESS)
  { std::cerr << "error : failed to begin session\n"; }
  else
  {
    std::cout << "xrBeginSession() == XR_SUCCESS\n";
  }

  return true;
}

void compute_projection_matrix_for_view(XrView* view, glm::mat4& output, float near_plane, float far_plane)
{
  output = glm::frustum(near_plane * glm::tan(view->fov.angleLeft),
                        near_plane * glm::tan(view->fov.angleRight),
                        near_plane * glm::tan(view->fov.angleDown),
                        near_plane * glm::tan(view->fov.angleUp),
                        near_plane,
                        far_plane);

  if(vr_system_openxr::need_to_vflip) output[1][1] *= -1.0f;
}

//Callback for the cameras
void left_eye_projection(glm::mat4& output, float near_plane, float far_plane)
{
  compute_projection_matrix_for_view(left_eye_view, output, near_plane, far_plane);
}
void right_eye_projection(glm::mat4& output, float near_plane, float far_plane)
{
  compute_projection_matrix_for_view(right_eye_view, output, near_plane, far_plane);
}

void vr_system_openxr::build_camera_node_system()
{
  //We are not using a head rig here, the views dorectly correspond to the cameras. We could do without the head node here
  head_node = vr_tracking_anchor->push_child(create_node());

  eye_camera_node[0] = head_node->push_child(create_node());
  eye_camera_node[1] = head_node->push_child(create_node());
  {
    camera l, r;
    l.vr_eye_projection_callback = left_eye_projection;
    r.vr_eye_projection_callback = right_eye_projection;
    l.projection_type            = camera::projection_mode::vr_eye_projection;
    r.projection_type            = camera::projection_mode::vr_eye_projection;
    eye_camera[0]                = eye_camera_node[0]->assign(std::move(l));
    eye_camera[1]                = eye_camera_node[1]->assign(std::move(r));
  }
}

void vr_system_openxr::wait_until_next_frame()
{
  //We are going to start a new frame, before that, we need to be able to sync up to the VR system to improve latency
  //Thus, we wait until the right time to start rendering
  XrFrameWaitInfo frame_wait_info;
  frame_wait_info.type = XR_TYPE_FRAME_WAIT_INFO;
  frame_wait_info.next = nullptr;
  zero_it(current_frame_state);
  current_frame_state.type = XR_TYPE_FRAME_STATE;

  if(auto status = xrWaitFrame(session, &frame_wait_info, &current_frame_state); status != XR_SUCCESS)
  { std::cerr << "Error while waiting for new frame " << NAMEOF_ENUM(status) << "\n"; }

  //Now the framestate contains the timing information for getting the view location, we can begin a new frame
  XrFrameBeginInfo frame_begin_info;
  zero_it(frame_begin_info);
  frame_begin_info.type = XR_TYPE_FRAME_BEGIN_INFO;
  xrBeginFrame(session, &frame_begin_info);
}

void vr_system_openxr::update_tracking()
{
  //Fetch the views informations
  XrViewState view_state;
  zero_it(view_state);
  view_state.type              = XR_TYPE_VIEW_STATE;
  uint32_t view_capacity_input = (uint32_t)views.size();
  uint32_t view_count;

  XrViewLocateInfo view_locate_info;
  zero_it(view_locate_info);
  view_locate_info.type                  = XR_TYPE_VIEW_LOCATE_INFO;
  view_locate_info.viewConfigurationType = used_view_configuration_type;
  view_locate_info.displayTime           = current_frame_state.predictedDisplayTime; //from xrWaitFrame
  view_locate_info.space                 = application_space;

  XrResult status = xrLocateViews(session, &view_locate_info, &view_state, view_capacity_input, &view_count, views.data());
  if(status != XR_SUCCESS) { std::cerr << NAMEOF_ENUM(status) << "\n"; }

  //From now, the two views contains up-to-date tracking info

  for(size_t i = 0; i < 2; ++i)
  {
    //There's a nasty pointer cast here that break a const, but it's for the greater good...
    //It just so happen that they both express vector 3D and quaternion in the same way...
    eye_camera_node[i]->local_xform.set_position(glm::make_vec3((float*)&views[i].pose.position));
    eye_camera_node[i]->local_xform.set_orientation(glm::make_quat((float*)&views[i].pose.orientation));
  }

  //This updates the world matrices on everybody
  vr_tracking_anchor->update_world_matrix();

  //This is requried to update the view matrix used for rendering on our cameras
  //The camera object is not aware it's attached to a node, it only know it has a view matrix.
  //Set world matrix on a camera update the view matrix by computing the inverse of the input
  for(size_t i = 0; i < 2; ++i) eye_camera[i]->set_world_matrix(eye_camera_node[i]->get_world_matrix());
}

#define static_array_size(static_array) sizeof(static_array) / sizeof(decltype(*static_array))

void vr_system_openxr::submit_frame_to_vr_system()
{
  XrResult status;

  //We have two swapchain to use:
  for(size_t i = 0; i < 2; i++)
  {
    uint32_t swapchain_index = -1;
    XrSwapchainImageAcquireInfo swapchain_image_acquire_info;
    zero_it(swapchain_image_acquire_info);
    swapchain_image_acquire_info.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;

    status = xrAcquireSwapchainImage(swapchain[i], &swapchain_image_acquire_info, &swapchain_index);
    if(status != XR_SUCCESS) { }

    XrSwapchainImageWaitInfo swapchain_image_wait_info;
    zero_it(swapchain_image_wait_info);
    swapchain_image_wait_info.type    = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
    swapchain_image_wait_info.timeout = 1000;

    status = xrWaitSwapchainImage(swapchain[i], &swapchain_image_wait_info);
    if(status != XR_SUCCESS) { }

    XrCompositionLayerProjectionView layer;
    zero_it(layer);
    layer.type                      = XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
    layer.pose                      = views[i].pose;
    layer.fov                       = views[i].fov;
    layer.subImage.swapchain        = swapchain[i];
    layer.subImage.imageRect.offset = { 0, 0 };
    layer.subImage.imageRect.extent = { eye_render_target_sizes[i].x, eye_render_target_sizes[i].y };

    //When this function has been called, the rendering of the frame to be pushed already occured,
    //we just need to copy the data in them.
#ifdef _WIN32
    if(!fallback_to_dx)
    {
#endif
      // clang-format off
      glCopyImageSubData(eye_render_texture[i],
                         GL_TEXTURE_2D, 0,0,0,0,
                         swapchain_images_opengl[i][swapchain_index].image,
                         GL_TEXTURE_2D, 0,0,0,0,
                         eye_render_target_sizes[i].x,
                         eye_render_target_sizes[i].y, 1);
      // clang-format on

#ifdef _WIN32
    }
    else
    {
      dx11_interop->copy(eye_render_texture[i], swapchain_images_d3d11[i][swapchain_index].texture, eye_render_target_sizes[i]);
    }
#endif

    projection_layer_views[i] = layer;

    XrSwapchainImageReleaseInfo swapchain_image_release_info;
    zero_it(swapchain_image_release_info);
    swapchain_image_release_info.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;

    status = xrReleaseSwapchainImage(swapchain[i], &swapchain_image_release_info);
    if(status != XR_SUCCESS) { }
  }
  glFlush();

  XrCompositionLayerProjection layer_projection;
  zero_it(layer_projection);
  layer_projection.type      = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
  layer_projection.space     = application_space;
  layer_projection.viewCount = 2;
  layer_projection.views     = projection_layer_views;
  layers[0]                  = ((XrCompositionLayerBaseHeader*)&layer_projection);

  XrFrameEndInfo frame_end_info;
  zero_it(frame_end_info);
  frame_end_info.type                 = XR_TYPE_FRAME_END_INFO;
  frame_end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
  frame_end_info.layerCount           = static_array_size(layers);
  frame_end_info.layers               = layers;
  frame_end_info.displayTime          = current_frame_state.predictedDisplayTime;

  status = xrEndFrame(session, &frame_end_info);
}
#endif
