#version 330 core

layout(location = 0) in vec2 p_in;
layout(location = 1) in vec2 uv_in;

out vec2 uv;

void main() 
{
  gl_Position = vec4(p_in.x, p_in.y, 0, 1);
  uv          = uv_in;
}