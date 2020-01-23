varying vec2 v_texcoords;
uniform sampler2D u_TextureSampler;

void main(void)
{
	vec4 texColor = texture2D(u_TextureSampler,	v_texcoords); 
	gl_FragColor = texColor;	
}