#version 330 core

out vec4 color_output;
in vec2 texture_coords;
uniform sampler2D in_texture;

void main()
{
	color_output = texture(in_texture, texture_coords);
}
