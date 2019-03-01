#version 330 core

//where we write the color of the fragment
out vec4 color_output;

//Where we read interpolated coordinates
in vec2 texture_coords;
in vec3 normal_dir;
in vec3 world_position;

uniform vec3 camera_position;
uniform mat4 view;
uniform mat4 model;

//where we read bound texture unit
uniform sampler2D in_texture;
uniform vec3 light_position_0;

vec3 ExtractCameraPos(mat4 a_modelView)
{
  // Get the 3 basis vector planes at the camera origin and transform them into model space.
  //  
  // NOTE: Planes have to be transformed by the inverse transpose of a matrix
  //       Nice reference here: http://www.opengl.org/discussion_boards/showthread.php/159564-Clever-way-to-transform-plane-by-matrix
  //
  //       So for a transform to model space we need to do:
  //            inverse(transpose(inverse(MV)))
  //       This equals : transpose(MV) - see Lemma 5 in http://mathrefresher.blogspot.com.au/2007/06/transpose-of-matrix.html
  //
  // As each plane is simply (1,0,0,0), (0,1,0,0), (0,0,1,0) we can pull the data directly from the transpose matrix.
  //  
  mat4 modelViewT = transpose(a_modelView);
 
  // Get plane normals 
  vec3 n1 = vec3(modelViewT[0]);
  vec3 n2 = vec3(modelViewT[1]);
  vec3 n3 = vec3(modelViewT[2]);
 
  // Get plane distances
  float d1 = (modelViewT[0].w);
  float d2 = (modelViewT[1].w);
  float d3 = (modelViewT[2].w);
 
  // Get the intersection of these 3 planes 
  // (uisng math from RealTime Collision Detection by Christer Ericson)
  vec3 n2n3 = cross(n2, n3);
  float denom = dot(n1, n2n3);
 
  vec3 top = (n2n3 * d1) + cross(n1, (d3*n2) - (d2*n3));
  return top / -denom;
}

void main()
{
	//light definition
	//vec3 light_position = vec3(-1, 3, 4);
	vec3 light_color = vec3(1, 0.82, 0.87);
	
	//ambiant light calculation
	float ambient_factor = 0.05;
	vec4 ambiant_color = vec4(ambient_factor * light_color, 1);

	//diffuse calculation
	vec3 normalized_normals = normalize(normal_dir);
	vec3 light_direction = normalize(light_position_0 - world_position);
	float diffuse_factor = max(dot(normalized_normals, light_direction), 0.0);
	vec3 diffuse_color = diffuse_factor * light_color;
	vec4 textured_diffuse_color = vec4(diffuse_color, 1) * texture(in_texture, texture_coords);

	//specular calculation
	//TODO get the view position from uniform ( = camera's position)
	vec3 view_dir = normalize(camera_position - world_position);
	vec3 reflect_dir = (reflect(-light_direction, normalized_normals));
	float spec = pow(max(dot(view_dir, reflect_dir), 0.0), 64);
	float specular_strengh = 0.01;
	vec4 specular_color = vec4(specular_strengh + spec * light_color, 1);

	color_output = ambiant_color + textured_diffuse_color + specular_color;
}
