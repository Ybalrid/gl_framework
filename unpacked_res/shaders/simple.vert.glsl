#version 330 core

//Vertex info
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;

//uniforms
uniform mat4 mvp; 

//fragment pass-through
out vec2 texture_coords;


void main()
{
	gl_Position = mvp * vec4(in_pos, 1.0);
	texture_coords = in_uv;
}
