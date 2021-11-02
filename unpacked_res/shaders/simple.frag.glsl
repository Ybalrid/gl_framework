#version 330 core
//where we write the color of the fragment
out vec4 color_output;

//Where we read interpolated coordinates
in vec2 texture_coordinates;
in vec3 normal_direction;
in vec3 world_position;
in mat3 TBN;
in vec4 light_space_position;

//cpu inputs
uniform vec3 camera_position;
uniform mat4 view;
uniform mat4 model;
uniform float gamma;

//Represent the object material
struct material_def
{
	sampler2D diffuse;
	sampler2D specular;
	sampler2D normal;

	float shininess;

	vec3 diffuse_color;
	vec3 specular_color;
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

#define PCF_OFFSET 2
#define PCF_AVERAGE 25

uniform sampler2D shadow_map;

vec4 apply_gamma(vec4 color, float gamma)
{
	return vec4(pow(color.rgb, vec3(1.0/gamma)), color.a);
}

vec3 calculate_directional_light(directional_light light, vec3 frag_normal, vec3 frag_view_direction, vec3 diffuse_sample_color, vec3 specular_sample_color);
vec3 calculate_point_light(point_light light, vec3 frag_normal, vec3 frag_world_position, vec3 frag_view_direction, vec3 diffuse_sample_color, vec3 specular_sample_color);

float compute_shadow_map(vec4 fragment_pos_light_space, float bias)
{
	//Project coordinates
	vec3 projected_coordinates = fragment_pos_light_space.xyz / fragment_pos_light_space.w;
	projected_coordinates = projected_coordinates * 0.5 +  0.5;

	float current_depth = projected_coordinates.z;

	//Compute shadow factor using PCF
	float shadow = 0.0;
	vec2 texel_size = 1.0 / textureSize(shadow_map, 0);
	//We sample 2 pixels on the side to deal with the lower resolution
	for(int x = -PCF_OFFSET; x <= PCF_OFFSET; ++x)
	{
		for(int y = -PCF_OFFSET; y <= PCF_OFFSET; ++y)
		{
			float pcf_depth = texture(shadow_map, projected_coordinates.xy + vec2(x, y) * texel_size).r;
			shadow += current_depth - bias > pcf_depth ? 1.0 : 0.0;
		}
	}
	shadow /= float(PCF_AVERAGE);

	return shadow;
}

void main()
{
	//compute additional vectors :
	vec3 normalized_normals = texture(material.normal, texture_coordinates).rgb;
	normalized_normals = normalize(normalized_normals * 2.0 - 1.0);
	normalized_normals = normalize(TBN * normalized_normals);

	vec3 view_direction = normalize(camera_position - world_position);

	vec4 diffuse_sample_color = texture(material.diffuse, texture_coordinates);
	if(diffuse_sample_color.a < 0.5) discard; //discard low alpha values
	vec4 specular_sample_color = texture(material.specular, texture_coordinates);

	//Accumulate each light contribution to shading
	vec3 color_result =  calculate_directional_light(main_directional_light, normalized_normals, view_direction, diffuse_sample_color.rgb, specular_sample_color.rgb);
	for(int i = 0; i < NB_POINT_LIGHTS; i++)
		color_result += calculate_point_light(point_light_list[i], normalized_normals, world_position, view_direction, diffuse_sample_color.rgb, specular_sample_color.rgb);

	//gamma correct the output:
	color_output = apply_gamma(vec4(color_result, 1.0), gamma);
}

vec3 calculate_directional_light(directional_light light, vec3 frag_normal, vec3 frag_view_direction, vec3 diffuse_sample_color, vec3 specular_sample_color)
{
	vec3 light_direction = normalize(-light.direction);

	//Use the shadow map to compute the fragment shadow mask
	float shadow_bias = max(0.005 * (1.0 - dot(frag_normal, light_direction)), 0.0005); //angle based bias
	float shadow = compute_shadow_map(light_space_position, shadow_bias);

	//calculate diffuse factor
	float diffuse_factor = max(dot(frag_normal, light_direction), 0.0);

	//calculate specular factor
	vec3 refection_direction = reflect(-light_direction, frag_normal);
	float specular_factor = pow(max(dot(frag_view_direction, refection_direction), 0.0), material.shininess);

	diffuse_sample_color = diffuse_sample_color * material.diffuse_color;
	specular_sample_color = specular_sample_color * material.specular_color;

	vec3 ambient_color = light.ambient * diffuse_sample_color;
	vec3 diffuse_color = light.diffuse * diffuse_factor * diffuse_sample_color;
	vec3 specular_color = light.specular * specular_factor * specular_sample_color;

	//Return the logit color contribution for the directional lamp
	return ambient_color + ((diffuse_color + specular_color) * vec3(1.0 - shadow)) ;
}

vec3 calculate_point_light(point_light light, vec3 frag_normal, vec3 frag_world_position, vec3 frag_view_direction, vec3 diffuse_sample_color, vec3 specular_sample_color)
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

	vec3 ambient_color = light.ambient * diffuse_sample_color;
	vec3 diffuse_color = light.diffuse * diffuse_factor * diffuse_sample_color;
	vec3 specular_color = light.specular * specular_factor * specular_sample_color;

	ambient_color *= attenuation;
	diffuse_color *= attenuation;
	specular_color *= attenuation;

	return ambient_color + diffuse_color + specular_color;
}
