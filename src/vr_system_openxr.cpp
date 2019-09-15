#include "vr_system_openxr.hpp"
#include "nameof.hpp"
#if USING_OPENXR
#endif
#define max_properties_count 64
#include "openxr/openxr_platform.h"

vr_system_openxr::~vr_system_openxr()
{
	if(session_started) xrEndSession(session);
	if(session_created) xrDestroySession(session);
	if(instance_created) xrDestroyInstance(instance);
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

bool vr_system_openxr::initialize()
{
	std::cout << "Initializing OpenXR based VR system\n";
	std::cout << getenv("XR_RUNTIME_JSON") << "\n";

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
		if(status = xrEnumerateInstanceExtensionProperties(
			   nullptr, max_properties_count, &extension_properties_count, extension_properties);
		   status != XR_SUCCESS)
		{ std::cerr << "error while enumerating instance extension properties\n"; }
	}
	std::vector<const char*> extension_properties_names((size_t)extension_properties_count);
	for(size_t i = 0; i < extension_properties_count; i++) extension_properties_names[i] = extension_properties[i].extensionName;

	//This is required to use OpenGL in OpenXR
	extension_properties_names.push_back("XR_KHR_opengl_enable");
	extension_properties_count++;

	//Create OpenXR instance
	const char engine_name[] = "the //TODO engine";
	XrInstanceCreateInfo instance_create_info;
	zero_it(instance_create_info);
	instance_create_info.applicationInfo.apiVersion = XR_MAKE_VERSION(1, 0, 0);
	strcpy(instance_create_info.applicationInfo.engineName, engine_name);
	strcpy(instance_create_info.applicationInfo.applicationName, GAME_NAME);
	instance_create_info.type				   = XR_TYPE_INSTANCE_CREATE_INFO;
	instance_create_info.enabledApiLayerCount  = api_layer_count;
	instance_create_info.enabledApiLayerNames  = api_layer_names.data();
	instance_create_info.enabledExtensionCount = extension_properties_count;
	instance_create_info.enabledExtensionNames = extension_properties_names.data();

	if(status = xrCreateInstance(&instance_create_info, &instance); status != XR_SUCCESS)
	{
		std::cerr << "error while creating XrInstance\n";
		return false; //we cannot recuperate from this
	}
	std::cout << "xrCreateInstance() == XR_SUCCESS\n";
	instance_created = true;

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

	//Get a HMD-style system
	XrSystemGetInfo system_get_info;
	zero_it(system_get_info);
	system_get_info.type	   = XR_TYPE_SYSTEM_GET_INFO;
	system_get_info.formFactor = XR_FORM_FACTOR_HEAD_MOUNTED_DISPLAY;
	if(status = xrGetSystem(instance, &system_get_info, &system_id); status != XR_SUCCESS)
	{ std::cerr << "error while getting system info for HMD form factor\n"; }

	//Retreive system properties
	XrSystemProperties system_properties;
	zero_it(system_properties);
	system_properties.type = XR_TYPE_SYSTEM_PROPERTIES;
	if(status = xrGetSystemProperties(instance, system_id, &system_properties); status != XR_SUCCESS)
	{ std::cerr << "error while getting system properties\n"; }

	std::cout << "HMD FormFactor system  : " << system_properties.systemName << "\n";
	std::cout << "Positional Tracking    : " << (system_properties.trackingProperties.positionTracking == XR_TRUE ? "YES" : "NO")
			  << "\n";
	std::cout << "Rotational Tracking    : "
			  << (system_properties.trackingProperties.orientationTracking == XR_TRUE ? "YES" : "NO") << "\n";
	std::cout << "Max Swapchain Size     : " << system_properties.graphicsProperties.maxSwapchainImageWidth << "x"
			  << system_properties.graphicsProperties.maxSwapchainImageWidth << "\n";
	std::cout << "Max layer count        : " << system_properties.graphicsProperties.maxLayerCount << "\n";

	//Retreive view configuration
	XrViewConfigurationType view_configuration_type[4];
	zero_it(view_configuration_type, 4);
	uint32_t view_configuration_type_count = 0;
	status = xrEnumerateViewConfigurations(instance, system_id, 4, &view_configuration_type_count, view_configuration_type);
	XrViewConfigurationType best_view_config_type = XR_VIEW_CONFIGURATION_TYPE_MAX_ENUM;
	if(view_configuration_type_count > 0) best_view_config_type = view_configuration_type[0];
	std::cout << "View configuration type : " << NAMEOF_ENUM(best_view_config_type) << "\n";

	XrViewConfigurationProperties view_configuration_properties;
	zero_it(view_configuration_properties);
	view_configuration_properties.type = XR_TYPE_VIEW_CONFIGURATION_PROPERTIES;
	if(status = xrGetViewConfigurationProperties(instance, system_id, best_view_config_type, &view_configuration_properties);
	   status != XR_SUCCESS)
	{ std::cerr << "error: cannot get view configuration properties\n"; }
	else
	{
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

	XrActionSetCreateInfo action_set_create_info;
	zero_it(action_set_create_info);
	action_set_create_info.type = XR_TYPE_ACTION_SET_CREATE_INFO;
	strcpy(action_set_create_info.actionSetName, "some_action_set");
	strcpy(action_set_create_info.localizedActionSetName, action_set_create_info.actionSetName);
	action_set_create_info.priority = -1;
	XrActionSet action_set;
	xrCreateActionSet(instance, &action_set_create_info, &action_set);

	//TODO xrSuggestInteractionProfileBindings()

	XrGraphicsRequirementsOpenGLKHR graphics_requirements_opengl_khr;
	zero_it(graphics_requirements_opengl_khr);
	graphics_requirements_opengl_khr.type = XR_TYPE_GRAPHICS_REQUIREMENTS_OPENGL_KHR;
	xrGetOpenGLGraphicsRequirementsKHR(instance, system_id, &graphics_requirements_opengl_khr);

	std::cout << "OpenGL version min required : " << XR_VERSION_MAJOR(graphics_requirements_opengl_khr.minApiVersionSupported)
			  << "." << XR_VERSION_MINOR(graphics_requirements_opengl_khr.minApiVersionSupported) << "\n";

	XrSessionCreateInfo session_create_info;
	zero_it(session_create_info);
	session_create_info.type	 = XR_TYPE_SESSION_CREATE_INFO;
	session_create_info.systemId = system_id;

#ifdef WIN32
	XrGraphicsBindingOpenGLWin32KHR xr_graphics_binding;
	zero_it(xr_graphics_binding);
	xr_graphics_binding.type  = XR_TYPE_GRAPHICS_BINDING_OPENGL_WIN32_KHR;
	xr_graphics_binding.hGLRC = wglGetCurrentContext();
	xr_graphics_binding.hDC	  = wglGetCurrentDC();
#else
//TODO linux X11 OpenGL
#endif
	session_create_info.next = &xr_graphics_binding;

	if(status = xrCreateSession(instance, &session_create_info, &session); status != XR_SUCCESS)
	{ std::cerr << "error: cannot create session " << NAMEOF_ENUM(status) << "\n"; }
	else
	{
		std::cout << "XrCreateSession() == XR_SUCCESS\n";
		session_created = true;
	}

	//reference spaces
	//action space
	//attach action set

	int64_t format[32];
	zero_it(format, 32);
	uint32_t format_count = 0;
	xrEnumerateSwapchainFormats(session, 32, &format_count, format);
	if(std::find(std::begin(format), std::end(format), GL_RGBA8) != std::end(format))
	{ std::cout << "found GL_RGBA8 in possible format list\n"; }
	else
	{
		std::cerr << "OpenGL texture format GL_RGBA8 not found within the supported formats by OpenXR runtime\n";
		return false;
	}

	for(size_t i = 0; i < 2; ++i)
	{
		XrSwapchainCreateInfo swapchain_create_info;
		zero_it(swapchain_create_info);
		swapchain_create_info.type		  = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		swapchain_create_info.usageFlags  = XR_SWAPCHAIN_USAGE_COLOR_ATTACHMENT_BIT | XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT;
		swapchain_create_info.mipCount	  = 1;
		swapchain_create_info.sampleCount = 1;
		swapchain_create_info.format	  = GL_RGBA8;
		swapchain_create_info.faceCount	  = 1;
		swapchain_create_info.arraySize	  = 1;
		swapchain_create_info.width		  = eye_render_target_sizes[i].x;
		swapchain_create_info.height	  = eye_render_target_sizes[i].y;

		xrCreateSwapchain(session, &swapchain_create_info, &swapchain[i]);

		uint32_t swapchain_image_count = 0;
		XrSwapchainImageOpenGLKHR swapchain_image_opengl_khr[8];
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
			{
				swapchain_images[i].push_back(swapchain_image_opengl_khr[img_index]);
				//auto w = eye_render_target_sizes[i].x;
				//auto h = eye_render_target_sizes[i].y;
				//glBindTexture(GL_TEXTURE_2D, GLuint(swapchain_image_opengl_khr[img_index].image));
				//glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
			}
		}
	}

	XrSessionBeginInfo xr_session_begin_info;
	zero_it(xr_session_begin_info);
	xr_session_begin_info.type						   = XR_TYPE_SESSION_BEGIN_INFO;
	xr_session_begin_info.primaryViewConfigurationType = best_view_config_type;
	if(status = xrBeginSession(session, &xr_session_begin_info); status != XR_SUCCESS)
	{ std::cerr << "error : failed to begin session\n"; }
	else
	{
		session_started = true;
		std::cout << "xrBeginSession() == XR_SUCCESS\n";
	}

	//OpenGL resource initialization
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
		//glGenerateMipmap(GL_TEXTURE_2D);
		glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, eye_render_texture[i], 0);

		glBindRenderbuffer(GL_RENDERBUFFER, eye_render_depth[i]);
		glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, w, h);
		glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, eye_render_depth[i]);

		if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{ std::cerr << "eye fbo " << i << " is not complete" << glCheckFramebufferStatus(GL_FRAMEBUFFER) << "\n"; }
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	return true;
}

void vr_system_openxr::build_camera_node_system()
{
	head_node = vr_tracking_anchor->push_child(create_node());

	eye_camera_node[0] = head_node->push_child(create_node());
	eye_camera_node[1] = head_node->push_child(create_node());
	{
		camera l, r;
		eye_camera[0] = eye_camera_node[0]->assign(std::move(l));
		eye_camera[1] = eye_camera_node[1]->assign(std::move(r));
	}
}

void vr_system_openxr::wait_until_next_frame()
{
	XrFrameWaitInfo frame_wait_info;
	frame_wait_info.type = XR_TYPE_FRAME_WAIT_INFO;
	frame_wait_info.next = nullptr;
	XrFrameState frame_state;
	zero_it(frame_state);
	frame_state.type = XR_TYPE_FRAME_STATE;

	if(auto status = xrWaitFrame(session, &frame_wait_info, &frame_state); status != XR_SUCCESS)
	{ std::cerr << "Error while waiting for new frame " << NAMEOF_ENUM(status) << "\n"; }

	XrFrameBeginInfo frame_begin_info;
	zero_it(frame_begin_info);
	frame_begin_info.type = XR_TYPE_FRAME_BEGIN_INFO;
	xrBeginFrame(session, &frame_begin_info);
}

void vr_system_openxr::update_tracking() { vr_tracking_anchor->update_world_matrix(); }

void vr_system_openxr::submit_frame_to_vr_system()
{
	XrResult status;
	for(size_t i = 0; i < 2; i++)
	{
		uint32_t index = -1;
		XrSwapchainImageAcquireInfo swapchain_image_acquire_info;
		zero_it(swapchain_image_acquire_info);
		swapchain_image_acquire_info.type = XR_TYPE_SWAPCHAIN_IMAGE_ACQUIRE_INFO;

		status = xrAcquireSwapchainImage(swapchain[i], &swapchain_image_acquire_info, &index);
		if(status != XR_SUCCESS) {}

		XrSwapchainImageWaitInfo swapchain_image_wait_info;
		zero_it(swapchain_image_wait_info);
		swapchain_image_wait_info.type	  = XR_TYPE_SWAPCHAIN_IMAGE_WAIT_INFO;
		swapchain_image_wait_info.timeout = 1000;

		status = xrWaitSwapchainImage(swapchain[i], &swapchain_image_wait_info);
		if(status != XR_SUCCESS) {}

		//std::cout << "image index for swapchain " << i << " is " << index << "\n";

		//At this point swapchain_image[i][index].image is the opengl texture to use,
		//but it seems the compositor is not initialized properly, so the following call fails:
		//glCopyImageSubData(eye_render_texture[i],
		//				   GL_TEXTURE_2D,
		//				   0,
		//				   0,
		//				   0,
		//				   0,
		//				   (GLuint)swapchain_images[i][index].image,
		//				   GL_TEXTURE_2D,
		//				   0,
		//				   0,
		//				   0,
		//				   0,
		//				   eye_render_target_sizes[i].x,
		//				   eye_render_target_sizes[i].y,
		//				   0);

		XrSwapchainImageReleaseInfo swapchain_image_release_info;
		zero_it(swapchain_image_release_info);
		swapchain_image_release_info.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;

		status = xrReleaseSwapchainImage(swapchain[i], &swapchain_image_release_info);
		if(status != XR_SUCCESS) {}
	}
	glFlush();

	XrFrameEndInfo frame_end_info;
	zero_it(frame_end_info);
	frame_end_info.type = XR_TYPE_FRAME_END_INFO;
	xrEndFrame(session, &frame_end_info);
}
