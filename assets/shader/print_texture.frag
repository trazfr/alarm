#version 100

/*
 * This fragment shader just prints the texture to the given coordinates
 */

precision mediump float;
varying vec2 v_texCoord;
uniform sampler2D s_texture;

void main()
{
    gl_FragColor = texture2D(s_texture, v_texCoord);
}
