#version 330 core

layout (location = 0) in vec3 input_position;

uniform mat4 projection;
uniform mat4 view;

out vec3 direction;

void main()
{
	direction = input_position;
	gl_Position = projection * view * vec4(input_position, 1.0);
}
