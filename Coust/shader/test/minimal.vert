#version 460

layout(set = 0, binding = 0) readonly buffer POSITION {
    vec3 positions_buf[];
};

layout(set = 0, binding = 1) readonly buffer INDEX {
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

layout(set = 0, binding = 2) readonly buffer ATTRIB_OFFSET {
    AttribOffsetArray attrib_offsets_buf[];
};

void main() {
    uint index = indices_buf[gl_VertexIndex];
    AttribOffsetArray offsets = attrib_offsets_buf[gl_InstanceIndex];
    vec3 pos = positions_buf[offsets.position_offset];
    gl_Position = vec4(pos, 1.0);
}
