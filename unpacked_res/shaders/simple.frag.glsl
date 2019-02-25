#version 330 core

out vec4 FragColor;
in vec2 texture_coords;
uniform sampler2D in_texture;

void main()
{
	FragColor = texture(in_texture, texture_coords);
}
