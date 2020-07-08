#version 130

uniform samplerCube u_SkyTexture;

varying vec3 v_texcoords;

void main(void)
{
	gl_FragColor = texture(u_SkyTexture, v_texcoords);
}