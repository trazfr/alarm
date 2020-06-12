#version 100

/*
 * This vertex shader uses the input position and pass the texture coordinate to the fragment shader
 * "print_texture.frag"
 */

attribute vec2 a_positionScreen;
attribute float a_textIndice;
varying vec2 v_texCoord;

const float glyphHeight = 32.0 / 256.0;
const float glyphWidth = 18.0 / 256.0;
const float indicesPerLine = floor(1. / glyphWidth) + 1.;

vec2 getTextCoord(float indice)
{
    float y = floor(indice / indicesPerLine);
    float x = indice - y * indicesPerLine;
    return vec2(x * glyphWidth, y * glyphHeight);
}

void main()
{
    gl_Position = vec4(a_positionScreen, 0.0, 1.0);
    v_texCoord = getTextCoord(a_textIndice);
}
