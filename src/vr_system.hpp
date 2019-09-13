#pragma once
#include "build_config.hpp"

#include <memory>
#include "node.hpp"
#include <glm/glm.hpp>

class vr_system
{
protected:
	node* camera_rig			 = nullptr;
	GLuint eye_fbo[2]			 = { 0, 0 };
	GLuint eye_render_texture[2] = { 0, 0 };
	GLuint eye_render_depth[2]	 = { 0, 0 };
	sdl::Vec2i eye_render_target_sizes[2] {};

public:
	vr_system() {}
	virtual ~vr_system() {}
	enum class eye { left, right };
	virtual bool initialize()						 = 0;
	virtual GLuint get_eye_framebuffer(eye)			 = 0;
	virtual sdl::Vec2i get_eye_framebuffer_size(eye) = 0;
	virtual void update_tracking()					 = 0;
	virtual void wait_until_next_frame()			 = 0;
	virtual void submit_frame_to_vr_system()		 = 0;
};

using vr_system_ptr = std::unique_ptr<vr_system>;

#if USING_OPENVR
//#include openvr based implementation of above interface
#include "vr_system_openvr.hpp"
#endif

#if USING_OCULUS_VR
//#include ovr based implementation of above interface
#endif

//TODO OpenHMD?
//TODO OpenXR?
