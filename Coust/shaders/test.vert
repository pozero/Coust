#version 460

#include "test.glsl"

layout (location = 0) in vec3 in_Pos;
layout (location = 1) in vec3 in_Color;

layout (location = 0) out vec3 out_Color;

void main()
{
    gl_Position = vec4(in_Pos, 1.0);
    out_Color = in_Color;
}
