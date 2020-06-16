#version 330 core

layout (location = 0) in vec2 input_position;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

uniform float debug_float_0;

void main() 
{
	gl_Position = projection * view * model * vec4(input_position, debug_float_0 * 0.00001, 1);
}