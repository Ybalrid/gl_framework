#version 330 core

out vec4 color_output;
uniform sampler2D splash;

in vec2 uv;

void main() 
{ 
	color_output = texture(splash, uv);
}