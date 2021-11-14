#pragma once

#include <FreeImage.h>

struct freeimage
{
  freeimage();
  ~freeimage();
  freeimage(const freeimage&) = delete;
  freeimage& operator=(const freeimage&) = delete;
  freeimage(freeimage&&)                 = delete;
  freeimage& operator=(freeimage&&) = delete;
};

struct freeimage_image
{
  freeimage_image() = default;
  freeimage_image(FIBITMAP* naked);
  ~freeimage_image();
  freeimage_image(const freeimage_image&) = delete;
  freeimage_image& operator=(const freeimage_image&) = delete;
  freeimage_image(freeimage_image&& o) noexcept;
  freeimage_image& operator=(freeimage_image&& o) noexcept;

  FIBITMAP* get() const;
  void set_ptr_to(FIBITMAP* naked);

  unsigned get_width() const;
  unsigned get_height() const;
  unsigned get_byte_per_pixel() const;
  unsigned char* get_raw_data() const;

private:
  FIBITMAP* ptr = nullptr;
};

struct freeimage_memory
{
  freeimage_memory() = default;
  freeimage_memory(unsigned char* memory, size_t size);
  freeimage_memory(FIMEMORY* naked);
  ~freeimage_memory();
  freeimage_memory(const freeimage_memory&) = delete;
  freeimage_memory& operator=(const freeimage_memory&) = delete;
  freeimage_memory(freeimage_memory&& o) noexcept;
  freeimage_memory& operator=(freeimage_memory&& o) noexcept;

  freeimage_image load();
  FIMEMORY* get() const;
  void set_ptr_to(FIMEMORY* new_ptr);

  private:
  FIMEMORY* ptr = nullptr;
};
