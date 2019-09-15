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
	XrInstance instance;
	XrSession session;
	XrSystemId system_id;
	XrSwapchain swapchain[2];
	std::vector<XrSwapchainImageOpenGLKHR> swapchain_images[2];

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
