#version 460
#extension GL_EXT_nonuniform_qualifier : require

layout(location = 0) in vec4 albedo_factor;
layout(location = 1) in vec2 albedo_texcoord;
layout(location = 2) flat in uint albedo_texture;

layout(location = 0) out vec4 out_color;

void main() {
    out_color = vec4(albedo_texcoord.xy, 0.0, 1.0);
}
