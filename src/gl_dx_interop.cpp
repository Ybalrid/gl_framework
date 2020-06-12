#include "gl_dx_interop.hpp"
#include <gl/wglew.h>
#include <iostream>

#ifdef _WIN32
gl_dx11_interop::gl_dx11_interop() { }

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



//Returns the last Win32 error, in string format. Returns an empty string if there is no error.
std::string GetLastErrorAsString()
{
  //Get the error message, if any.
  DWORD errorMessageID = ::GetLastError();
  if(errorMessageID == 0) return std::string(); //No error message has been recorded

  LPSTR messageBuffer = nullptr;
  size_t size = FormatMessageA(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                               NULL,
                               errorMessageID,
                               MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                               (LPSTR)&messageBuffer,
                               0,
                               NULL);

  std::string message(messageBuffer, size);

  //Free the buffer.
  LocalFree(messageBuffer);

  return message;
}



bool gl_dx11_interop::copy(GLuint gl_image_source, ID3D11Texture2D* dx_texture_dst, sdl::Vec2i viewport)
{
  //TODO need a texture cache
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
  if(FAILED(result))
  {
    return false;
  }

  HANDLE shared_handle = 0;
  IDXGIResource* resource = nullptr;
  intermediate_texture->QueryInterface(__uuidof(IDXGIResource), (void**)&resource);
  if(resource)
  resource->GetSharedHandle(&shared_handle);

  GLuint dx_texture_gl;
  glGenTextures(1, &dx_texture_gl);
  BOOL set_shared_handle_result = wglDXSetResourceShareHandleNV(intermediate_texture, shared_handle);
  HANDLE interop_object
      = wglDXRegisterObjectNV(gl_dx_device, intermediate_texture, dx_texture_gl, GL_TEXTURE_2D, WGL_ACCESS_READ_WRITE_NV);

  if(interop_object == nullptr)
  {
    const auto Win32Error = GetLastErrorAsString();
    std::cerr << Win32Error << std::endl;
    intermediate_texture->Release();
    glDeleteTextures(1, &dx_texture_gl);
    return false;
  }

  wglDXLockObjectsNV(gl_dx_device, 1, &interop_object);
  glCopyImageSubData(
      gl_image_source, GL_TEXTURE_2D, 0, 0, 0, 0, dx_texture_gl, GL_TEXTURE_2D, 0, 0, 0, 0, viewport.x, viewport.y, 1);
  wglDXUnlockObjectsNV(gl_dx_device, 1, &interop_object);
  wglDXUnregisterObjectNV(gl_dx_device, interop_object);


  glDeleteTextures(1, &dx_texture_gl);

  context->CopyResource(dx_texture_dst, intermediate_texture);
  intermediate_texture->Release();
  return true;
}

#endif
