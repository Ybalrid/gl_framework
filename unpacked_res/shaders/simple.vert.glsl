#version 330 core

//Vertex info
layout (location = 0) in vec3 input_position;
layout (location = 1) in vec2 input_texture_coordinates;
layout (location = 2) in vec3 input_normal;
layout (location = 3) in vec3 input_tangent;

//uniforms
uniform mat4 mvp;		//Projection * View * Model
uniform mat4 model;		//Model
uniform mat4 view;		//View
uniform mat3 normal;	//transpose(inverse(Model))

//fragment pass-through
out vec2 texture_coordinates;
out vec3 normal_direction;
out vec3 world_position;
out mat3 TBN;

void main()
{
	//pass through theses parameters to be interpolated
	texture_coordinates = input_texture_coordinates;
	world_position = vec3(model * vec4(input_position, 1.0)); //This will permit to get the position of a fragment in world space
	normal_direction = normal * input_normal;				  //Using the normal matrix to correct the normal direction for rotation/scale in world space
	
	//TODO compute bitangent CPU side... 
	vec3 input_bitangent = cross(input_normal, input_tangent);
	//vec3 bitangent = normal * input_normal;

	vec3 T = normalize(vec3(model * vec4(input_tangent,   0.0)));
	vec3 B = normalize(vec3(model * vec4(input_bitangent, 0.0)));
	vec3 N = normalize(vec3(model * vec4(input_normal,    0.0)));

	TBN = mat3(T,B,N);


	//perform the main projection from world space to normalized_device_coordinates	
	gl_Position = mvp * vec4(input_position, 1.0);
}
