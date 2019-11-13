#version 150

uniform sampler2D texture;
uniform bool use_matrix;
uniform int eye;
uniform vec2 extent_texcrd;
uniform vec2 center_left;
uniform vec2 center_right;

in vec4 tex_position;
in vec2 texcoord_fs;

out vec4 frag_color;

void main()
{
	vec2 texcrd = tex_position.xy / tex_position.w;
	texcrd.y = -texcrd.y;

	vec2 center = center_right;
	if (eye == 0)
		center = center_left;

	texcrd= texcrd*extent_texcrd+center;
	if (use_matrix)
		frag_color = texture2D(texture, texcrd);
	else 
		frag_color = texture2D(texture, texcoord_fs);
}