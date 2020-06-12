#include "gl_dx_interop.hpp"
#include <gl/wglew.h>
#include <iostream>

#ifdef _WIN32
gl_dx11_interop::gl_dx11_interop()
{
  //TODO we probably want this to be a singleton, it holds resources that should be unique
}

gl_dx11_interop::~gl_dx11_interop()
{
  for(auto& [id3d11texture, shared_texure] : gl_dx_share_cache)
  {
    wglDXUnregisterObjectNV(gl_dx_device, shared_texure.interop_object);
    glDeleteTextures(1, &shared_texure.intermediate_texture_glid);
    shared_texure.intermediate_texture->Release();
  }

  gl_dx_share_cache.clear();
  wglDXCloseDeviceNV(gl_dx_device);

  swapchain->Release();
  context->Release();
  device->Release();
}

bool gl_dx11_interop::init()
{

  instance                   = GetModuleHandleW(nullptr);
  WNDCLASSW window_class     = { 0 };
  window_class.style         = CS_OWNDC;
  window_class.lpfnWndProc   = DefWindowProcW;
  window_class.hInstance     = instance;
  window_class.lpszClassName = dummy_window_class;
  RegisterClassW(&window_class);

  dummy_window = CreateWindowExW(0,
                                 dummy_window_class,
                                 L"gldxinteropwindow",
                                 WS_OVERLAPPED,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 nullptr,
                                 nullptr,
                                 instance,
                                 nullptr);

  D3D_FEATURE_LEVEL dxlevels[] = { D3D_FEATURE_LEVEL_11_1, D3D_FEATURE_LEVEL_11_0 };
  D3D_FEATURE_LEVEL used_level;

  DXGI_SWAP_CHAIN_DESC desc = {};
  desc.BufferDesc.Format    = DXGI_FORMAT_R8G8B8A8_UNORM;
  desc.SampleDesc.Count     = 1;
  desc.BufferUsage          = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  desc.BufferCount          = 2;
  desc.OutputWindow         = dummy_window;
  desc.Windowed             = true;

  const auto result = D3D11CreateDeviceAndSwapChain(nullptr,
                                                    D3D_DRIVER_TYPE_HARDWARE,
                                                    nullptr,
                                                    0,
                                                    dxlevels,
                                                    2,
                                                    D3D11_SDK_VERSION,
                                                    &desc,
                                                    &swapchain,
                                                    &device,
                                                    &used_level,
                                                    &context);

  if(FAILED(result))
  {
    std::cerr << "Did not create D3D11Context!\n";
    return false;
  }

  if(!wglewIsSupported("WGL_NV_DX_interop"))
  {
    std::cerr << "WGL_NV_DX_interop is not supported by the current OpenGL context!\n";
    return false;
  }

  gl_dx_device = wglDXOpenDeviceNV(device);

  return gl_dx_device != nullptr;
}

void gl_dx11_interop::perform_copy(GLuint gl_image_source,
                                   ID3D11Texture2D* dx_texture_dst,
                                   sdl::Vec2i viewport,
                                   shared_texture& sh_txt) const
{
  //Copy GL texture to DX11 intermediate texture (via OpenGL)
  wglDXLockObjectsNV(gl_dx_device, 1, &sh_txt.interop_object);
  // clang-format off
  glCopyImageSubData(gl_image_source, 
                     GL_TEXTURE_2D, 
                     0, 0, 0, 0, 
                     sh_txt.intermediate_texture_glid, 
                     GL_TEXTURE_2D,
                     0, 0, 0, 0,
                     viewport.x,
                     viewport.y, 1);
  // clang-format on
  wglDXUnlockObjectsNV(gl_dx_device, 1, &sh_txt.interop_object);

  //Copy intermediate texture to destination (via D3D11)
  context->CopyResource(dx_texture_dst, sh_txt.intermediate_texture);
}

bool gl_dx11_interop::copy(GLuint gl_image_source, ID3D11Texture2D* dx_texture_dst, sdl::Vec2i viewport)
{
  const auto cached_texture = gl_dx_share_cache.find(dx_texture_dst);
  if(cached_texture == std::end(gl_dx_share_cache))
  {
    D3D11_TEXTURE2D_DESC intermediate_texture_description = {};
    intermediate_texture_description.Width                = viewport.x;
    intermediate_texture_description.Height               = viewport.y;
    intermediate_texture_description.MipLevels            = 1;
    intermediate_texture_description.ArraySize            = 1;
    intermediate_texture_description.Format               = DXGI_FORMAT_R8G8B8A8_UNORM_SRGB;
    intermediate_texture_description.SampleDesc.Count     = 1;
    intermediate_texture_description.Usage                = D3D11_USAGE_DEFAULT;
    intermediate_texture_description.BindFlags            = 0;
    intermediate_texture_description.MiscFlags            = D3D11_RESOURCE_MISC_SHARED;

    ID3D11Texture2D* intermediate_texture = nullptr;
    const auto result = device->CreateTexture2D(&intermediate_texture_description, nullptr, &intermediate_texture);
    if(FAILED(result)) { return false; }

    HANDLE shared_handle    = 0;
    IDXGIResource* resource = nullptr;
    intermediate_texture->QueryInterface(__uuidof(IDXGIResource), (void**)&resource);
    if(resource) resource->GetSharedHandle(&shared_handle);

    GLuint dx_texture_gl;
    glGenTextures(1, &dx_texture_gl);
    const BOOL set_shared_handle_result = wglDXSetResourceShareHandleNV(intermediate_texture, shared_handle);
    if(set_shared_handle_result == FALSE)
    {
      intermediate_texture->Release();
      glDeleteTextures(1, &dx_texture_gl);
      return false;
    }

    const HANDLE interop_object
        = wglDXRegisterObjectNV(gl_dx_device, intermediate_texture, dx_texture_gl, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);

    if(interop_object == nullptr)
    {
      intermediate_texture->Release();
      glDeleteTextures(1, &dx_texture_gl);
      return false;
    }

    shared_texture sh_txt            = {};
    sh_txt.interop_object            = interop_object;
    sh_txt.intermediate_texture_glid = dx_texture_gl;
    sh_txt.shared_handle             = shared_handle;
    sh_txt.intermediate_texture      = intermediate_texture;

    gl_dx_share_cache[dx_texture_dst] = sh_txt;

    if(copy(gl_image_source, dx_texture_dst, viewport)) { return true; }
    std::cerr << "did not register shared texture properly\n";
    return false;
  }

  auto sh_txt = cached_texture->second;

  perform_copy(gl_image_source, dx_texture_dst, viewport, sh_txt);
  return true;
}

#endif
