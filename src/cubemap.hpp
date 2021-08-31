#pragma once

#include <array>
#include <GL/glew.h>

#include "image.hpp"

class cubemap
{

public:
  cubemap(const std::array<image, 6>& faces)
  {
    const GLenum type = [&] { switch(faces[0].get_type())
    {
        case image::type::rgb: return GL_RGB;
        case image::type::rgba: return GL_RGBA;
    }}();

    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_CUBE_MAP, texture);

    for(int i = 0; i < faces.size(); ++i)
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
          0,
          static_cast<GLint>(type),
          faces[i].get_width(),
          faces[i].get_height(),
          0,
          type,
          GL_UNSIGNED_BYTE,
          static_cast<const void*>(faces[i].get_binary()));

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

  }

~cubemap()
  {
    glDeleteTextures(1, &texture);
    texture = 0;
  }

  GLuint get_texture() const
  { return texture;
  }
private:
  GLuint texture{};
};
