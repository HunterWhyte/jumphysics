#version 100

attribute vec4 a_position;
attribute vec2 a_texCoord0;
attribute vec4 a_color;

varying vec2 v_texcoord;
varying vec4 v_color;
uniform mat4 u_projTrans;

void main() {
    v_texcoord = a_texCoord0;
    v_color = a_color;
    gl_Position =  u_projTrans * a_position;
}