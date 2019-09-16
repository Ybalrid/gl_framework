#pragma once
#include "vr_system.hpp"
#if USING_OPENXR

//Configure XR headers
#define XR_USE_GRAPHICS_API_OPENGL
#ifdef WIN32
#define XR_USE_PLATFORM_WIN32
#else
//TODO linux here
#endif

#include <openxr/openxr.h>
#include <openxr/openxr_platform.h>
class vr_system_openxr : public vr_system
{
	XrInstance instance		 = XR_NULL_HANDLE;
	XrSession session		 = XR_NULL_HANDLE;
	XrSystemId system_id	 = 0;
	XrSwapchain swapchain[2] = { 0 };
	XrViewConfigurationType used_view_configuration_type;
	std::vector<XrView> views;
	std::vector<XrSwapchainImageOpenGLKHR> swapchain_images[2];
	XrFrameState current_frame_state;
	std::vector<XrCompositionLayerBaseHeader*> layers;
	XrCompositionLayerProjectionView projection_layer_views[2];
	XrSpace application_space = XR_NULL_HANDLE;

	//TODO these are useless the handles can be compared against XR_NULL_HANDLE
	bool instance_created = false;
	bool session_created  = false;
	bool session_started  = false;

	node* eye_camera_node[2] = { nullptr, nullptr };

public:
	vr_system_openxr() = default;

	virtual ~vr_system_openxr();
	bool initialize() override;
	void build_camera_node_system() override;
	void wait_until_next_frame() override;
	void update_tracking() override;
	void submit_frame_to_vr_system() override;
};
#endif
