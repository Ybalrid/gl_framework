#version 330 core

out vec4 color_output;
in vec2 texture_coords;
uniform sampler2D in_texture;

uniform float gamma;

vec4 apply_gamma(vec4 color, float gamma_value)
{
	return vec4(pow(color.rgb, vec3(1.0/gamma)), color.a);
}

//just return the color sampled from the texture without any shading
void main()
{
	color_output = apply_gamma(texture(in_texture, texture_coords), gamma);
}