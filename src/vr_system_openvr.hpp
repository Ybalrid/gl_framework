#pragma once

#include "vr_system.hpp"
#if USING_OPENVR

#include "openvr.h"

class vr_system_openvr : public vr_system
{
	vr::IVRSystem* hmd																 = nullptr;
	vr::TrackedDevicePose_t tracked_device_pose_array[vr::k_unMaxTrackedDeviceCount] = { 0 };
	bool init_success																 = false;
	vr::Texture_t texture_handlers[2];

public:
	vr_system_openvr();

	virtual ~vr_system_openvr();

	bool initialize() override;

	void deinitialize_openvr();

	GLuint get_eye_framebuffer(eye) override;

	sdl::Vec2i get_eye_framebuffer_size(eye) override;

	void update_tracking() override;
	void wait_until_next_frame() override;
	void submit_frame_to_vr_system() override;
};

#endif
