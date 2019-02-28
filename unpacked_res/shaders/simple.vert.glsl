#version 330 core

//Vertex info
layout (location = 0) in vec3 in_pos;
layout (location = 1) in vec2 in_uv;
layout (location = 2) in vec3 in_norm;

//uniforms
uniform mat4 mvp; 
uniform mat4 model;
uniform mat4 view;
uniform mat3 normal;

//fragment pass-through
out vec2 texture_coords;
out vec3 normal_dir;
out vec3 world_position;

void main()
{
	gl_Position = mvp * vec4(in_pos, 1.0);
	texture_coords = in_uv;
	world_position = vec3(model * vec4(in_pos, 1.0));
	normal_dir = normal * in_norm;
}
