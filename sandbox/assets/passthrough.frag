#version 100

precision mediump float;

varying vec2 v_texcoord;
varying vec4 v_color;

uniform sampler2D u_sampler2D;

void main() {
    vec4 color = texture2D(u_sampler2D, v_texcoord);
    gl_FragColor = color*v_color;
}
