#version 420
attribute vec3 a_position;
attribute vec2 a_texcoords;
attribute vec3 a_normals;

varying vec3 v_fragPos;
varying vec2 v_texcoords;
varying vec3 v_normals;
varying vec4 ShadowCoord;

uniform float u_time;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_modelMatrixLight;
uniform sampler2D u_ShadowMap;

void main(void)
{
	v_texcoords = a_texcoords;
	v_normals = mat3(transpose(inverse(u_modelMatrix))) * a_normals;
	v_normals = normalize(v_normals);
	v_fragPos = vec3(u_modelMatrix * vec4(a_position , 1.0));
	ShadowCoord = u_modelMatrixLight * vec4(a_position , 1.0);

	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * vec4(a_position , 1.0);
}