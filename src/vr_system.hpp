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

	node* head_node	   = nullptr;
	node* hand_node[2] = { nullptr, nullptr };

public:
	//Nothing special to do in ctor/dtor
	vr_system()			 = default;
	virtual ~vr_system() = default;

	enum class eye { left, right };

	//--- the following is generic and should work as-is for all vr systems
	///Get the eye frame buffers
	GLuint get_eye_framebuffer(eye);
	///Get the size of the framebuffers in pixel
	sdl::Vec2i get_eye_framebuffer_size(eye);
	///Set this pointer to become the root of the tracked object's sub-tree
	void set_anchor(node* node);
	///get the camera to use for rendering
	camera* get_eye_camera(eye output);

	//--- the following is the abstract interface that require implementation specific work
	///Call this once OpenGL is fully setup. This function ini the VR and create the textures, framebuffers
	virtual bool initialize() = 0;
	///Only call this after `set_anchor` to a non-null pointer
	virtual void build_camera_node_system() = 0;
	///Needs to be called before any rendering, sync with the headset. On some platform, this also acquire tracking states
	virtual void wait_until_next_frame() = 0;
	///Move scene nodes to match VR tracking
	virtual void update_tracking() = 0;
	///Send content of the framebuffers to the VR system
	virtual void submit_frame_to_vr_system() = 0;
};

using vr_system_ptr = std::unique_ptr<vr_system>;

#if USING_OPENVR
//#include openvr based implementation of above interface
#include "vr_system_openvr.hpp"
#endif

#if USING_OCULUS_VR
//#include ovr based implementation of above interface
#endif

#if USING_OPENXR
#include "vr_system_openxr.hpp"
#endif

//TODO OpenHMD?
//TODO OpenXR?
