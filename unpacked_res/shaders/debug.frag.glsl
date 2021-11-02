#version 330 core

out vec4 color_output;

uniform vec4 debug_color;

void main()
{
	color_output = debug_color;
}