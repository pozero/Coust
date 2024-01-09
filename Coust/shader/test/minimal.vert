#version 460

#define POS_STRIDE 3
#define TEXCOORD_STRIDE 2

layout(std430, set = 0, binding = 0) readonly buffer VERTEX {
    float vertex_buf[];
};

layout(std430, set = 1, binding = 0) readonly buffer INDEX {
    uint indices_buf[];
};

struct AttribOffsetArray {
    uint position_offset;
    uint normal_offset;
    uint tangent_offset;
    uint color_offset0;
    uint texcoord_offset[4];
};

layout(std430, set = 1, binding = 1) readonly buffer ATTRIB_OFFSET {
    AttribOffsetArray attrib_offsets_buf[];
};

layout(std430, set = 2, binding = 0) readonly buffer RESULT_MATRICES {
    mat4 matrices[];
};

struct Material {
    vec4 albedo_factor;
    uint albedo_texture;
    uint albedo_texcoord;
};

layout(std430, set = 3, binding = 0) readonly buffer MATERIAL_INDEX {
    uint material_indices[];
};

layout(std430, set = 3, binding = 1) readonly buffer MATERIALS {
    Material material_buf[];
};

layout(push_constant, std430) uniform PC {
    mat4 proj_view_mat;
    uint matrix_idx;
};

layout(location = 0) out vec4 albedo_factor;
layout(location = 1) out vec2 albedo_texcoord;
layout(location = 2) flat out uint albedo_texture;

void main() {
    uint index = indices_buf[gl_VertexIndex];
    AttribOffsetArray offsets = attrib_offsets_buf[gl_InstanceIndex];
    vec4 pos = vec4(
        vertex_buf[offsets.position_offset + POS_STRIDE * index + 0],
        vertex_buf[offsets.position_offset + POS_STRIDE * index + 1],
        vertex_buf[offsets.position_offset + POS_STRIDE * index + 2], 1.0);
    pos = proj_view_mat * matrices[matrix_idx] * pos;
    gl_Position = pos;

    uint material_idx = material_indices[gl_InstanceIndex];
    Material material = material_buf[material_idx];
    albedo_factor = material.albedo_factor;
    albedo_texcoord = vec2(vertex_buf[offsets.texcoord_offset[material.albedo_texcoord] + 
                                   TEXCOORD_STRIDE * index + 0],
                               vertex_buf[offsets.texcoord_offset[material.albedo_texcoord] + 
                                   TEXCOORD_STRIDE * index + 1]);
    albedo_texture = material.albedo_texture;
}
