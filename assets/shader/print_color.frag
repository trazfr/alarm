#version 100

/*
 * This fragment shader applies a constant color to the fragment
 */

precision mediump float;
uniform vec3 u_color;

void main()
{
    gl_FragColor = vec4(u_color, 1.);
}
