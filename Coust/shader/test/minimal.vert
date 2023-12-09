#version 460

#define POS_STRIDE 3

layout(std430, set = 0, binding = 0) readonly buffer POSITION {
    float positions_buf[];
};

layout(std430, set = 0, binding = 1) readonly buffer INDEX {
    uint indices_buf[];
};

struct AttribOffsetArray {
    uint position_offset;
    uint normal_offset;
    uint tangent_offset;
    uint color_offset0;
    uint texcoord_offset0;
    uint texcoord_offset1;
    uint texcoord_offset2;
    uint texcoord_offset3;
};

layout(std430, set = 0, binding = 2) readonly buffer ATTRIB_OFFSET {
    AttribOffsetArray attrib_offsets_buf[];
};

layout(std430, set = 0, binding = 3) readonly buffer RESULT_MATRICES {
    mat4 matrices[];
};

layout(push_constant, std430) uniform PC {
    mat4 proj_view_mat;
    uint mat_idx;
};

void main() {
    uint index = indices_buf[gl_VertexIndex];
    AttribOffsetArray offsets = attrib_offsets_buf[gl_InstanceIndex];
    vec4 pos = vec4(
        positions_buf[offsets.position_offset + POS_STRIDE * index + 0],
        positions_buf[offsets.position_offset + POS_STRIDE * index + 1],
        positions_buf[offsets.position_offset + POS_STRIDE * index + 2], 1.0);
    pos = proj_view_mat * matrices[mat_idx] * pos;
    gl_Position = pos;
}
