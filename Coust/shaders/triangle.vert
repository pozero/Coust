#version 460

vec3 positions[3] = vec3[3]
(
	vec3(-0.5f,  0.5f, 0.0f),
	vec3( 0.5f,  0.5f, 0.0f),
	vec3( 0.0f, -0.5f, 0.0f)
);

vec3 colors[3] = vec3[3]
(
    vec3(1.0f, 0.0f, 0.0f),
    vec3(0.0f, 1.0f, 0.0f),
    vec3(0.0f, 0.0f, 1.0f)
);

vec2 tex[3] = vec2[3]
(
    vec2(0.0f, 0.0f),
    vec2(1.0f, 1.0f),
    vec2(1.0f, 0.0f)
);

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec2 outTexCoord;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f); 
    outColor = colors[gl_VertexIndex];
    outTexCoord = tex[gl_VertexIndex];
}
