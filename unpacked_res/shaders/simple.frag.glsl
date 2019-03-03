#version 330 core
//where we write the color of the fragment
out vec4 color_output;

//Where we read interpolated coordinates
in vec2 texture_coordinates;
in vec3 normal_direction;
in vec3 world_position;

//cpu inputs
uniform vec3 camera_position;
uniform mat4 view;
uniform mat4 model;
uniform float gamma;

struct material_def
{
	sampler2D diffuse;
	sampler2D specular;
	float shininess;
};
uniform material_def material;

struct directional_light
{
	vec3 direction;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
uniform directional_light main_directional_light;

struct point_light
{
	vec3 position;

	float constant;
	float linear;
	float quadratic;

	vec3 ambient;
	vec3 diffuse;
	vec3 specular;
};
#define NB_POINT_LIGHTS 4
uniform point_light point_light_list[NB_POINT_LIGHTS];


vec4 apply_gamma(vec4 color, float gamma)
{
	return vec4(pow(color.rgb, vec3(1.0/gamma)), color.a);
}

vec3 calculate_directional_light(directional_light light, vec3 frag_normal, vec3 frag_view_direction);
vec3 calculate_point_light(point_light light, vec3 frag_normal, vec3 frag_world_position, vec3 frag_view_direction);

void main()
{
	//compute additional vectors : 
	vec3 normalized_normals = normalize(normal_direction);
	vec3 view_direction = normalize(camera_position - world_position);

	//Accumulate each light contribution to shading
	vec3 color_result =  calculate_directional_light(main_directional_light, normalized_normals, view_direction);
	for(int i = 0; i < NB_POINT_LIGHTS; i++)
		color_result += calculate_point_light(point_light_list[i], normalized_normals, world_position, view_direction);

	//gamma correct the output:
	color_output = apply_gamma(vec4(color_result, 1.0), gamma);
}

vec3 calculate_directional_light(directional_light light, vec3 frag_normal, vec3 frag_view_direction)
{
	vec3 light_direction = normalize(-light.direction);

	//calculate diffuse factor
	float diffuse_factor = max(dot(frag_normal, light_direction), 0.0);
	
	//calculate specular factor
	vec3 refection_direction = reflect(-light_direction, frag_normal);
	float specular_factor = pow(max(dot(frag_view_direction, refection_direction), 0.0), material.shininess);

	vec3 diffuse_sample_color = texture(material.diffuse, texture_coordinates).rgb;
	//TODO material system that permit to have a specular texture 
	vec3 specular_sample_color = diffuse_sample_color;
	
	
	vec3 ambient_color = light.ambient * diffuse_sample_color;
	vec3 diffuse_color = light.diffuse * diffuse_factor * diffuse_sample_color;
	vec3 specular_color = light.specular * specular_factor * specular_sample_color;

	//Return this fragment shaded by this one light
	return ambient_color + diffuse_color + specular_color;
}

vec3 calculate_point_light(point_light light, vec3 frag_normal, vec3 frag_world_position, vec3 frag_view_direction)
{
	vec3 light_direction = normalize(light.position - frag_world_position);

	//calculate diffuse factor
	float diffuse_factor = max(dot(frag_normal, light_direction), 0.0);
	
	//calculate specular factor
	vec3 reflection_direction = reflect(-light_direction, frag_normal);
	float specular_factor = pow(max(dot(frag_view_direction, reflection_direction), 0.0), material.shininess);
	//calculate attenuation
	float light_frag_distance = length(light.position - frag_world_position);
	float light_frag_distance_sq = light_frag_distance*light_frag_distance;
	float attenuation = 1.0 / (
		(light.constant)
		+ (light.linear * (light_frag_distance))
		+ (light.quadratic * (light_frag_distance_sq))
	);

	vec3 diffuse_sample_color = texture(material.diffuse, texture_coordinates).rgb;
	vec3 specular_sample_color = texture(material.specular, texture_coordinates).rgb;

	vec3 ambient_color = light.ambient * diffuse_sample_color;
	vec3 diffuse_color = light.diffuse * diffuse_factor * diffuse_sample_color;
	vec3 specular_color = light.specular * specular_factor * specular_sample_color;

	ambient_color *= attenuation;
	diffuse_color *= attenuation;
	specular_color *= attenuation;

	return ambient_color + diffuse_color + specular_color;
}
