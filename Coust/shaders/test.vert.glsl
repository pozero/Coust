#version 460

layout (location = 0, component = 0) in float v0;
layout (location = 0, component = 1) in float v1;
layout (location = 0, component = 2) in vec2 v2;

layout (location = 1) in mat4 m0[2];

layout (location = 9) in float t1;

void main()
{
    gl_Position = vec4(v0, v1, v2);
}