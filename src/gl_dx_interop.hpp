#pragma once
#ifdef _WIN32

#include <gl/glew.h>
#include <cpp-sdl2/sdl.hpp>
#include <d3d11.h>
#include <dxgi.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "dxgi.lib")

class gl_dx11_interop
{
  public:

  struct shared_texture
  {
    ID3D11Texture2D* texture;
    HANDLE shared_handle;
  };

  gl_dx11_interop();
  gl_dx11_interop(const gl_dx11_interop&) = delete;
  gl_dx11_interop& operator=(const gl_dx11_interop&) = delete;

  bool init();

  ID3D11Device* get_device() const
  {
    return device;
  }

  bool copy(GLuint gl_image_source, ID3D11Texture2D* dx_texture_dst, sdl::Vec2i viewport);

  private:
  ID3D11Device* device;
  ID3D11DeviceContext* context;
  IDXGISwapChain* swapchain;
  const wchar_t* dummy_window_class = L"gldx11interopwindowclass";
  HWND dummy_window                 = nullptr;
  HINSTANCE instance                = nullptr;
  HANDLE gl_dx_device               = nullptr;


};

#endif
