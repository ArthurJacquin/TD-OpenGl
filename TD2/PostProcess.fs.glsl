#version 120

varying vec2 v_texcoords;
uniform sampler2D u_TextureSampler;

void main(void)
{
	vec4 texColor = texture2D(u_TextureSampler,	v_texcoords); 
	const vec3 luminanceWeight = vec3 (0.2125, 0.7154, 0.072); 
	float luminance = dot(texColor.rgb, luminanceWeight);
	vec3 lumVector = vec3(luminance);

	gl_FragColor = texColor;	
}