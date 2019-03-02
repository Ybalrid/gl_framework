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
uniform sampler2D in_texture;
uniform vec3 light_position_0;

vec4 apply_gamma(vec4 color, float gamma)
{
	return vec4(pow(color.rgb, vec3(1.0/gamma)), color.a);
}

void main()
{
	//Sample the texture : 
	vec4 textured_color = texture(in_texture, texture_coordinates);

	//TODO read this from uniform!
	vec3 light_color = vec3(1,1,1); //Warm-ish white
	float ambient_factor = 0.3;
	float constant = 1.0;
	float linear = 0.09;
	float quadratic = 0.032;
	float specular_strengh = 0.2;
	float specular_power = 32;

	//compute additional vectors : 
	vec3 normalized_normals = normalize(normal_direction);
	vec3 light_direction = normalize(light_position_0 - world_position);
	vec3 view_direction = normalize(camera_position - world_position);
	vec3 reflect_direction = reflect(-light_direction, normalized_normals);
	float light_distance = length(light_position_0 - world_position);

	//compute ambiant, diffuse and specular factors, and distance attenuation
	float diffuse_factor = max(dot(normalized_normals, light_direction), 0.0);
	float specular_factor = pow(max(dot(view_direction, reflect_direction), 0.0), specular_power);
	float attenuation = 1.0 / (constant + (linear * light_distance) + (quadratic * (light_distance * light_distance)));
	
	//compute the color of the differnt kinds of light
	vec4 ambiant_color = vec4(ambient_factor * light_color, 1);
	vec4 diffuse_color = vec4(diffuse_factor * light_color, 1);
	vec4 specular_color = vec4(specular_strengh * specular_factor * light_color, 1);

	//compute the color from the current fragment's diffuse texture
	vec4 textured_diffuse_color = textured_color * diffuse_color;
	vec4 textured_ambiant_color = textured_color * ambiant_color;
	vec4 textured_specular_color = textured_color * specular_color;

	//allumulate the result, and apply gammma correction
	color_output = apply_gamma(textured_ambiant_color * attenuation  
	+ textured_diffuse_color * attenuation 
	+ specular_color * attenuation, gamma);
}
