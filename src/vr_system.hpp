#pragma once
#include "build_config.hpp"

#include <memory>
#include "node.hpp"
#include "transform.hpp"

class vr_system
{
	node_ptr camera_rig;

	GLuint eye_fbo[2];
	GLuint eye_render_texture[2];

public:
	enum class eye { left, right };
	virtual bool initialize()					 = 0;
	virtual GLuint get_eye_framebuffer(eye)		 = 0;
	virtual glm::vec2 get_eye_framebuffer_size() = 0;
	virtual void update_tracking()				 = 0;
};

using vr_system_ptr = std::unique_ptr<vr_system>;

#if USING_OPENVR
//#include openvr based implementation of above interface
#endif

#if USING_OCULUS_VR
//#include ovr based implementation of above interface
#endif

//TODO OpenHMD?
//TODO OpenXR?
