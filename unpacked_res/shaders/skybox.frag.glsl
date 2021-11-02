#version 330 core

in vec3 direction;
uniform samplerCube cubemap;

out vec4 frag_color;

void main()
{
	frag_color = texture(cubemap, direction);
}