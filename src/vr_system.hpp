#pragma once
#include <cpp-sdl2/sdl.hpp>
#include "build_config.hpp"

#include <memory>
#include "node.hpp"
#include <glm/glm.hpp>

//This is the interface for all VR systems
class vr_system
{
protected:
	///Any VR system needs a reference point in the scene to sync tracking between real and virtual world
	node* vr_tracking_anchor = nullptr;
	///Pair or camera objects to render in stereoscopy
	camera* eye_camera[2] = { nullptr, nullptr };
	///Framebuffer objects to render to texture
	GLuint eye_fbo[2] = { 0, 0 };
	///Render texture
	GLuint eye_render_texture[2] = { 0, 0 };
	///Render buffer for depth
	GLuint eye_render_depth[2] = { 0, 0 };
	///Size of the above buffers in pixels
	sdl::Vec2i eye_render_target_sizes[2] {};

public:
	//Nothing special to do in ctor/dtor
	vr_system()			 = default;
	virtual ~vr_system() = default;

	enum class eye { left, right };

	///Call this once OpenGL is fully setup
	virtual bool initialize() = 0;

	virtual GLuint get_eye_framebuffer(eye)			 = 0;
	virtual sdl::Vec2i get_eye_framebuffer_size(eye) = 0;
	virtual void update_tracking()					 = 0;
	virtual void wait_until_next_frame()			 = 0;
	virtual void submit_frame_to_vr_system()		 = 0;

	///Only call this after `set_anchor` to a non-null pointer
	virtual void build_camera_node_system() = 0;

	///Set this pointer to become the root of the tracked object's sub-tree
	void set_anchor(node* node);
	///get the camera to use for rendering
	camera* get_eye_camera(eye output);
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
