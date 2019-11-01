#pragma once

#include "resource_system.hpp"
#include "freeimage_raii.hpp"
#include <GL/glew.h>
#include <stdexcept>

class image
{
  ///RAII managed FreeImage object
  freeimage_image internal_image;

  ///Make the Image an RGB image (BRG/RGB and odd format convert)
  void rgbize_bitmap();

  ///Move operation helper
  void steal_guts(image& other) { internal_image = std::move(other.internal_image); }

  public:
  ///constructor
  explicit image(const std::string& virtual_path);

  ///Destructor
  ~image() = default;

  ///No copy
  image(const image&) = delete;

  ///No copy
  image& operator=(const image&) = delete;

  ///move constructor
  image(image&& other) noexcept;

  ///move operator
  image& operator=(image&& other) noexcept;

  ///Get the width of the image
  int get_width() const;

  ///Get the height of the image
  int get_height() const;

  ///Alpha or no alpha
  enum class type { rgba, rgb };

  ///Get the OpenGL type
  static GLint get_gl_type(type t);

  ///Get the type (RGB or RGBA
  type get_type() const;

  ///Get this juicy byte array :O
  uint8_t* get_binary() const;
};
