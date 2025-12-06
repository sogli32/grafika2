#version 330 core

layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTex;

out vec2 TexCoord;

uniform vec2 uCenter;
uniform vec2 uScale;
uniform vec2 uOffset;   // samo za karakter

void main()
{
    vec2 scaled = aPos * uScale;
    vec2 world  = scaled + uCenter + uOffset;

    gl_Position = vec4(world, 0.0, 1.0);
    TexCoord = aTex;
}
