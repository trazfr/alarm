#version 100

/*
 * This vertex shader prints a clock hand.
 * It is basically drawing the vertex after a rotation around an axis 
 */

attribute vec2 a_positionScreen;
attribute float a_rotationFactor;
uniform vec2 u_rotationAxis;
uniform vec2 u_squareScreenFactor;
uniform highp float u_rotation;

#define PI 3.14159265359

mat2 getRotationMatrix(float angle)
{
    float c = cos(angle) * u_squareScreenFactor[0];
    float s = sin(angle) * u_squareScreenFactor[1];
    return mat2(c, -s,
                s, c);
}

vec2 rotate(vec2 uv, vec2 pivot, float angle)
{
    // the geometry is initialized at 0,0
    return getRotationMatrix(angle) * uv + pivot;
}

void main()
{
    vec2 posOGL = rotate(a_positionScreen, u_rotationAxis, mod(u_rotation * a_rotationFactor, 1.) * (2.*PI));
    gl_Position = vec4(posOGL, 0.0, 1.0);
}
