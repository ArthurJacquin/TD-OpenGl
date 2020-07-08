#version 130

attribute vec3 a_position;
varying vec3 v_texcoords;

uniform mat4 u_projectionMatrix;
uniform mat4 u_viewMatrix;

void main(void)
{
    v_texcoords = vec3(a_position.x, -a_position.y, a_position.z);
    gl_Position =  vec4(a_position, 1.0); //u_projectionMatrix * u_viewMatrix *
}