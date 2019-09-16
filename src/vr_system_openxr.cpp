#include "vr_system_openxr.hpp"
#include "nameof.hpp"
#if USING_OPENXR
#endif
#define max_properties_count 64
#include "openxr/openxr_platform.h"

//That's a bit ugly I know
XrView *left_eye_view = nullptr, *right_eye_view = nullptr;

vr_system_openxr::~vr_system_openxr()
{
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
	used_view_configuration_type = best_view_config_type;

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
	XrGraphicsBindingOpenGLXlibKHR xr_graphics_binding;
	zero_it(xr_graphics_binding);
	//TODO test on linux X11 OpenGL with glXGetCurrentContext(); and glXGetCurrentDrawable(); Probably SDL syswminfo too
	this_is_broken_right_here();
#endif
	session_create_info.next = &xr_graphics_binding;

	if(status = xrCreateSession(instance, &session_create_info, &session); status != XR_SUCCESS)
	{ std::cerr << "error: cannot create session " << NAMEOF_ENUM(status) << "\n"; }
	else
	{
		std::cout << "XrCreateSession() == XR_SUCCESS\n";
	}

	//reference space
	XrPosef identity_pose;
	zero_it(identity_pose);
	identity_pose.orientation.w = 1;
	XrReferenceSpaceCreateInfo reference_space_create_info;
	zero_it(reference_space_create_info);
	reference_space_create_info.type				 = XR_TYPE_REFERENCE_SPACE_CREATE_INFO;
	reference_space_create_info.referenceSpaceType	 = XR_REFERENCE_SPACE_TYPE_LOCAL;
	reference_space_create_info.poseInReferenceSpace = identity_pose;
	zero_it(application_space);
	if(status = xrCreateReferenceSpace(session, &reference_space_create_info, &application_space); status != XR_SUCCESS)
	{ std::cerr << NAMEOF_ENUM(status) << "\n"; }

	//Create swapchains
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
		swapchain_create_info.type = XR_TYPE_SWAPCHAIN_CREATE_INFO;
		swapchain_create_info.usageFlags
			= XR_SWAPCHAIN_USAGE_TRANSFER_DST_BIT; //Transfer Destination bit. We only ever transfer rendered images to this, we don't use it as part of the rendering.
		swapchain_create_info.mipCount	  = 1;
		swapchain_create_info.sampleCount = 1;
		swapchain_create_info.format
			= GL_SRGB8_ALPHA8; //TODO check the spec about SRGB/Linear color spaces. The missmatch here is intentional, image's too bright without this
		swapchain_create_info.faceCount = 1;
		swapchain_create_info.arraySize = 1;
		swapchain_create_info.width		= eye_render_target_sizes[i].x;
		swapchain_create_info.height	= eye_render_target_sizes[i].y;

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
			{ swapchain_images[i].push_back(swapchain_image_opengl_khr[img_index]); }
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

void compute_projection_matrix_for_view(XrView* view, glm::mat4& output, float near_plane, float far_plane)
{
	output = glm::frustum(near_plane * glm::tan(view->fov.angleLeft),
						  near_plane * glm::tan(view->fov.angleRight),
						  near_plane * glm::tan(view->fov.angleDown),
						  near_plane * glm::tan(view->fov.angleUp),
						  near_plane,
						  far_plane);
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
	head_node = vr_tracking_anchor->push_child(create_node());

	eye_camera_node[0] = head_node->push_child(create_node());
	eye_camera_node[1] = head_node->push_child(create_node());
	{
		camera l, r;
		l.vr_eye_projection_callback = left_eye_projection;
		r.vr_eye_projection_callback = right_eye_projection;
		l.projection_type			 = camera::projection_mode::eye_vr;
		r.projection_type			 = camera::projection_mode::eye_vr;
		eye_camera[0]				 = eye_camera_node[0]->assign(std::move(l));
		eye_camera[1]				 = eye_camera_node[1]->assign(std::move(r));
	}
}

void vr_system_openxr::wait_until_next_frame()
{
	XrFrameWaitInfo frame_wait_info;
	frame_wait_info.type = XR_TYPE_FRAME_WAIT_INFO;
	frame_wait_info.next = nullptr;
	zero_it(current_frame_state);
	current_frame_state.type = XR_TYPE_FRAME_STATE;

	if(auto status = xrWaitFrame(session, &frame_wait_info, &current_frame_state); status != XR_SUCCESS)
	{ std::cerr << "Error while waiting for new frame " << NAMEOF_ENUM(status) << "\n"; }

	//Frame state contai

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
	view_state.type				 = XR_TYPE_VIEW_STATE;
	uint32_t view_capacity_input = views.size();
	uint32_t view_count;

	XrViewLocateInfo view_locate_info;
	zero_it(view_locate_info);
	view_locate_info.type				   = XR_TYPE_VIEW_LOCATE_INFO;
	view_locate_info.viewConfigurationType = used_view_configuration_type;
	view_locate_info.displayTime		   = current_frame_state.predictedDisplayTime;
	view_locate_info.space				   = application_space;

	XrResult status = xrLocateViews(session, &view_locate_info, &view_state, view_capacity_input, &view_count, views.data());
	if(status != XR_SUCCESS) { std::cerr << NAMEOF_ENUM(status) << "\n"; }

	//From now, the two views contains up-to-date tracking info

	for(size_t i = 0; i < 2; ++i)
	{
		//Thre's a nasty pointer cast here that break a const, but it's for the greater good...
		//It just so happen that they both express vector 3D and quaternion in the same way...
		eye_camera_node[i]->local_xform.set_position(glm::make_vec3((float*)&views[i].pose.position.x));
		eye_camera_node[i]->local_xform.set_orientation(glm::make_quat((float*)&views[i].pose.orientation));
	}

	//This updates the world matrices on everybody
	vr_tracking_anchor->update_world_matrix();

	//This is requried to update the view matrix used for renderign
	for(size_t i = 0; i < 2; ++i) eye_camera[i]->set_world_matrix(eye_camera_node[i]->get_world_matrix());
}

void vr_system_openxr::submit_frame_to_vr_system()
{
	XrResult status;

	//TODO this is unfortunate, we don't really need to clear and recreate this thing.
	//to be honest, it will only ever grew up to one, so...
	layers.clear();

	//We have two swapchain to use:
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

		XrCompositionLayerProjectionView layer;
		zero_it(layer);
		layer.type						= XR_TYPE_COMPOSITION_LAYER_PROJECTION_VIEW;
		layer.pose						= views[i].pose;
		layer.fov						= views[i].fov;
		layer.subImage.swapchain		= swapchain[i];
		layer.subImage.imageRect.offset = { 0, 0 };
		layer.subImage.imageRect.extent = { eye_render_target_sizes[i].x, eye_render_target_sizes[i].y };

		//We **finally** got access to the texture we have to render to.
		GLuint dst = swapchain_images[i][index].image;

		//When this function has been called, the rendering of the frame to be pushed already occured,
		//we just need to copy the data in them.
		glCopyImageSubData(eye_render_texture[i],
						   GL_TEXTURE_2D,
						   0,
						   0,
						   0,
						   0,
						   dst,
						   GL_TEXTURE_2D,
						   0,
						   0,
						   0,
						   0,
						   eye_render_target_sizes[i].x,
						   eye_render_target_sizes[i].y,
						   1);

		projection_layer_views[i] = layer;

		XrSwapchainImageReleaseInfo swapchain_image_release_info;
		zero_it(swapchain_image_release_info);
		swapchain_image_release_info.type = XR_TYPE_SWAPCHAIN_IMAGE_RELEASE_INFO;

		status = xrReleaseSwapchainImage(swapchain[i], &swapchain_image_release_info);
		if(status != XR_SUCCESS) {}
	}
	glFlush();

	XrCompositionLayerProjection layer_projection;
	zero_it(layer_projection);
	layer_projection.type	   = XR_TYPE_COMPOSITION_LAYER_PROJECTION;
	layer_projection.space	   = application_space;
	layer_projection.viewCount = 2;
	layer_projection.views	   = projection_layer_views;
	layers.push_back((XrCompositionLayerBaseHeader*)&layer_projection);

	XrFrameEndInfo frame_end_info;
	zero_it(frame_end_info);
	frame_end_info.type					= XR_TYPE_FRAME_END_INFO;
	frame_end_info.environmentBlendMode = XR_ENVIRONMENT_BLEND_MODE_OPAQUE;
	frame_end_info.layerCount			= layers.size();
	frame_end_info.layers				= layers.data();

	status = xrEndFrame(session, &frame_end_info);
}
