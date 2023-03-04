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

layout (location = 0) out vec3 outColor;

void main()
{
    gl_Position = vec4(positions[gl_VertexIndex], 1.0f); 
    outColor = colors[gl_VertexIndex];
}
