#version 460

layout (location = 0) in vec3 inColor;
layout (location = 1) in vec2 inTexCoord;

layout (location = 0) out vec4 outColor;

layout (set = 0, binding = 0) uniform sampler2D tex1;

void main()
{
    outColor = vec4(texture(tex1, inTexCoord).xyz + inColor, 1.0);
}
