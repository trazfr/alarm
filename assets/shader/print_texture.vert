#version 100

/*
 * This vertex shader is used to print a sprite
 */

attribute vec2 a_positionScreen;
attribute vec2 a_texCoord;
varying vec2 v_texCoord;

void main()
{
    gl_Position = vec4(a_positionScreen, 0.0, 1.0);
    v_texCoord = a_texCoord;
}
