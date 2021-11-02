#version 330 core

layout (location = 0) in vec3 world_position;
layout (location = 1) in vec4 vertex_color;

uniform mat4 view;
uniform mat4 projection;

out vec4 color;

void main()
{
	color = vertex_color;
	gl_Position = projection * view * vec4(world_position, 1);
}
