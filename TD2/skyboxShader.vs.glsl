#version 130

attribute vec3 a_position;
attribute vec2 a_texcoords;

varying vec3 v_texcoords;

uniform mat4 projection;
uniform mat4 view;

void main(void)
{
    v_texcoords = a_position;
    gl_Position = vec4(a_position, 1.0);
}