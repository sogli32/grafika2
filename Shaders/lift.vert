#version 330 core

layout (location = 0) in vec2 aPos;

uniform float uLeftX;
uniform float uRightX;
uniform float uTopY;
uniform float uBottomY;

void main()
{
    vec2 pos = aPos;

    float x = mix(uLeftX, uRightX, pos.x);
    float y = mix(uBottomY, uTopY, pos.y);

    gl_Position = vec4(x, y, 0.0, 1.0);
}
