#include "freeimage_raii.hpp"
#include <iostream>

freeimage::freeimage()
{
  FreeImage_Initialise();

  std::cout << "Initialized FreeImage " << FreeImage_GetVersion() << "\n";
}

freeimage::~freeimage()
{
  FreeImage_DeInitialise();
  std::cout << "Deinitialized FreeImage\n";
}

freeimage_image::freeimage_image(FIBITMAP* naked) : ptr(naked) {}

freeimage_image::~freeimage_image()
{
  if(ptr) FreeImage_Unload(ptr);
}

FIBITMAP* freeimage_image::get() const { return ptr; }

void freeimage_image::set_ptr_to(FIBITMAP* naked)
{
  if(ptr) FreeImage_Unload(ptr);
  ptr = naked;
}

unsigned freeimage_image::get_width() const { return FreeImage_GetWidth(ptr);}
unsigned freeimage_image::get_height() const { return FreeImage_GetHeight(ptr);}
unsigned freeimage_image::get_byte_per_pixel() const { return FreeImage_GetBPP(ptr);}
unsigned char* freeimage_image::get_raw_data() const {return FreeImage_GetBits(ptr);}

freeimage_image::freeimage_image(freeimage_image&& o) noexcept
{
  ptr   = o.ptr;
  o.ptr = nullptr;
}

freeimage_image& freeimage_image::operator=(freeimage_image&& o) noexcept
{
  ptr   = o.ptr;
  o.ptr = nullptr;
  return *this;
}

freeimage_memory::freeimage_memory(unsigned char* memory, size_t size) : ptr(FreeImage_OpenMemory(memory, (DWORD)size)) {}

freeimage_memory::freeimage_memory(FIMEMORY* naked) : ptr(naked) {}

freeimage_memory::~freeimage_memory()
{
  if(ptr) FreeImage_CloseMemory(ptr);
}

FIMEMORY* freeimage_memory::get() const { return ptr; }

void freeimage_memory::set_ptr_to(FIMEMORY* new_ptr)
{
  if(ptr) FreeImage_CloseMemory(ptr);
  ptr = new_ptr;
}

freeimage_memory::freeimage_memory(freeimage_memory&& o) noexcept
{
  ptr   = o.ptr;
  o.ptr = nullptr;
}

freeimage_memory& freeimage_memory::operator=(freeimage_memory&& o) noexcept
{
  ptr   = o.ptr;
  o.ptr = nullptr;
  return *this;
}

freeimage_image freeimage_memory::load()
{
  const auto type = FreeImage_GetFileTypeFromMemory(ptr);
  return { FreeImage_LoadFromMemory(type, ptr) };
}
