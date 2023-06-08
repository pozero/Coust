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

void main() {
    uint index = indices_buf[gl_VertexIndex];
    AttribOffsetArray offsets = attrib_offsets_buf[gl_InstanceIndex];
    gl_Position = vec4(
        positions_buf[POS_STRIDE * (offsets.position_offset + index) + 0],
        positions_buf[POS_STRIDE * (offsets.position_offset + index) + 1],
        positions_buf[POS_STRIDE * (offsets.position_offset + index) + 2], 1.0);
}
