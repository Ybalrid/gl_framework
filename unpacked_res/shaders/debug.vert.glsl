#version 330 core

layout (location = 0) in vec3 world_position;

uniform mat4 view;
uniform mat4 projection;

void main()
{
	gl_Position = projection * view * vec4(world_position, 1);
}