#version 460

layout(location = 0) out vec4 outColor;

void main() {
    outColor = gl_FragCoord;
}
