#include "image.hpp"

void image::rgbize_bitmap()
{
  const auto blue_mask      = FreeImage_GetBlueMask(internal_image.get());
  const auto red_mask       = FreeImage_GetRedMask(internal_image.get());
  const auto bits_per_pixel = FreeImage_GetBPP(internal_image.get());
  const auto bits           = FreeImage_GetBits(internal_image.get());
  const auto w              = get_width();
  const auto h              = get_height();
  const auto px_count       = w * h;

  if(bits_per_pixel == 32 && red_mask > blue_mask)
  {
    auto* data_array = reinterpret_cast<uint32_t*>(bits);
    for(size_t i = 0; i < px_count; ++i)
    {
      const auto pixel = data_array[i];
      const auto a     = 0xff000000 & pixel;
      const auto r     = 0x00ff0000 & pixel;
      const auto g     = 0x0000ff00 & pixel;
      const auto b     = 0x000000ff & pixel;

      data_array[i] = (a) | (b << 16) | (g) | (r >> 16);
    }
  }
}

image::image(const std::string& virtual_path)
{
  auto image_data = resource_system::get_file(virtual_path);

  freeimage_memory image_memory(FreeImage_OpenMemory(image_data.data(), (unsigned long)(image_data.size())));
  const auto image_type = FreeImage_GetFileTypeFromMemory(image_memory.get());
  if(image_type == FIF_UNKNOWN)
    throw std::runtime_error("The data from " + virtual_path + " doesn't seem to be a readable image");

  internal_image = image_memory.load();
  image_data.clear();

  if(!internal_image.get()) throw std::runtime_error("Couldn't load bitmap from " + virtual_path);

  internal_image = FreeImage_ConvertTo32Bits(internal_image.get());
  FreeImage_FlipVertical(internal_image.get());

  rgbize_bitmap();
}

image::image(image&& other) noexcept { steal_guts(other); }

image& image::operator=(image&& other) noexcept
{
  steal_guts(other);
  return *this;
}

int image::get_width() const { return FreeImage_GetWidth(internal_image.get()); }

int image::get_height() const { return FreeImage_GetHeight(internal_image.get()); }

GLint image::get_gl_type(type t)
{
  switch(t)
  {
    case type::rgb: return GL_RGB;
    case type::rgba: return GL_RGBA;
    default: throw std::runtime_error("cannot make sence of your type thing");
  }
}

image::type image::get_type() const
{
  if(FreeImage_GetBPP(internal_image.get()) == 32) return type::rgba;
  return type::rgb;
}

uint8_t* image::get_binary() const { return FreeImage_GetBits(internal_image.get()); }
