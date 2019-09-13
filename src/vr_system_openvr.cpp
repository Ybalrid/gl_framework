#include <cpp-sdl2/sdl.hpp>
#include <iostream>
#include "vr_system_openvr.hpp"
#include <gl/glew.h>

#if USING_OPENVR
vr_system_openvr::vr_system_openvr() : vr_system() { std::cout << "Initialized OpenVR based vr_system implementation\n"; }
vr_system_openvr::~vr_system_openvr()
{
	std::cout << "Deinitialized OpenVR based vr_system implementation\n";

	deinitialize_openvr();
}

bool vr_system_openvr::initialize()
{
	//Pre-init fastchecks
	if(!vr::VR_IsRuntimeInstalled())
	{
		sdl::show_message_box(SDL_MESSAGEBOX_INFORMATION,
							  "Please install SteamVR",
							  "You are attempting to use an OpenVR application "
							  "without having installed the runtime");
		return false;
	}

	if(!vr::VR_IsHmdPresent())
	{
		sdl::show_message_box(SDL_MESSAGEBOX_INFORMATION,
							  "Please connect an HMD",
							  "This application cannot detect an HMD being present. Will not start VR");
		return false;
	}

	//Init attempt
	vr::EVRInitError error;
	hmd = vr::VR_Init(&error, vr::EVRApplicationType::VRApplication_Scene);

	//Init of OpenVR sucessfull
	if(!hmd)
	{
		sdl::show_message_box(SDL_MESSAGEBOX_ERROR, "Error: Cannot start OpenVR", VR_GetVRInitErrorAsSymbol(error));
		return false;
	}

	if(!vr::VRCompositor())
	{
		sdl::show_message_box(
			SDL_MESSAGEBOX_ERROR, "Error: OpenVR compositor is not accessible", "Please check that SteamVR is running properly.");
		return false;
	}

	//Create eye render textures
	glGenTextures(2, eye_render_texture);
	glGenRenderbuffers(2, eye_render_depth);
	glGenFramebuffers(2, eye_fbo);
	for(size_t i = 0; i < 2; i++)
	{
		//Acquire textures sizes
		uint32_t w, h;
		hmd->GetRecommendedRenderTargetSize(&w, &h);
		eye_render_target_sizes[i].x = w;
		eye_render_target_sizes[i].y = h;

		std::cout << "Initializing FBO for eye " << i << " with pixel size " << w << "x" << h << "\n";

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

		//Build texture handles for SteamVR
		texture_handlers[i].eColorSpace = vr::EColorSpace::ColorSpace_Auto;
		texture_handlers[i].eType		= vr::ETextureType::TextureType_OpenGL;
		texture_handlers[i].handle		= (void*)eye_render_texture[i];
	}
	//Unbound any left bound framebuffers
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	init_success = true;
	return init_success;
}

void vr_system_openvr::deinitialize_openvr()
{
	//TODO proper cleanup
}

GLuint vr_system_openvr::get_eye_framebuffer(eye output)
{
	if(output == eye::left) return eye_fbo[0];
	if(output == eye::right) return eye_fbo[1];
}

sdl::Vec2i vr_system_openvr::get_eye_framebuffer_size(eye output)
{
	if(output == eye::left) return eye_render_target_sizes[0];
	if(output == eye::right) return eye_render_target_sizes[1];
}

void vr_system_openvr::update_tracking()
{
	for(size_t i = 0; i < vr::k_unMaxTrackedDeviceCount; ++i)
	{
		//handle device with index "i"
	}
}

void vr_system_openvr::wait_until_next_frame()
{
	vr::VRCompositor()->WaitGetPoses(tracked_device_pose_array, vr::k_unMaxTrackedDeviceCount, nullptr, 0);
}

#include "imgui.h"

void vr_system_openvr::submit_frame_to_vr_system()
{
	const auto left_error  = vr::VRCompositor()->Submit(vr::Eye_Left, &texture_handlers[0]);
	const auto right_error = vr::VRCompositor()->Submit(vr::Eye_Right, &texture_handlers[1]);
	glFlush();

	if(left_error != vr::VRCompositorError_None) { std::cerr << (int)left_error << "\n"; }
	if(right_error != vr::VRCompositorError_None) { std::cerr << (int)right_error << "\n"; }
}
#endif
