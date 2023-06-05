#include "pch.h"

#include "render/vulkan/VulkanVertex.h"

namespace coust {
namespace render {

std::array<uint32_t, 3> constexpr related_queues{
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
};

VulkanVertexIndexBuffer::VulkanVertexIndexBuffer(VkDevice dev,
    VmaAllocator alloc, VkCommandBuffer cmdbuf,
    class VulkanStagePool& stage_pool,
    MeshAggregate const& mesh_aggregate) noexcept
    : m_vertex_buf(dev, alloc,
          mesh_aggregate.vertex_buffer.size() *
              sizeof(decltype(MeshAggregate::vertex_buffer)::value_type),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues),
      m_index_buf(dev, alloc,
          mesh_aggregate.index_buffer.size() *
              sizeof(decltype(MeshAggregate::index_buffer)::value_type),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues),
      m_draw_cmd_buf(dev, alloc,
          sizeof(VkDrawIndirectCommand) *
              MeshAggregate::get_primitve_count(mesh_aggregate),
          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues),
      m_attrib_offset_buf(dev, alloc,
          sizeof(Mesh::Primitive::attrib_offset) *
              MeshAggregate::get_primitve_count(mesh_aggregate),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues) {
    for (uint32_t i = 0; i < mesh_aggregate.attrib_bytes_offset.size(); ++i) {
        size_t const attrib_offset_in_byte =
            mesh_aggregate.attrib_bytes_offset[i];
        if (attrib_offset_in_byte == INVALID_VERTEX_ATTRIB) {
            continue;
        }
        size_t const range_in_byte =
            i == mesh_aggregate.attrib_bytes_offset.size() - 1 ?
                m_vertex_buf.get_size() - attrib_offset_in_byte :
                mesh_aggregate.attrib_bytes_offset[i + 1] -
                    attrib_offset_in_byte;
        VkDescriptorBufferInfo info{
            .buffer = m_vertex_buf.get_handle(),
            .offset = attrib_offset_in_byte,
            .range = range_in_byte,
        };
        m_attrib_info.emplace((VertexAttrib) i, info);
    }
    m_vertex_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) mesh_aggregate.vertex_buffer.data(),
            m_vertex_buf.get_size()});
    m_index_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) mesh_aggregate.index_buffer.data(),
            m_index_buf.get_size()});
    size_t const primitive_cnt =
        MeshAggregate::get_primitve_count(mesh_aggregate);
    memory::vector<VkDrawIndirectCommand, DefaultAlloc> draw_cmds{
        get_default_alloc()};
    draw_cmds.reserve(primitive_cnt);
    memory::vector<decltype(Mesh::Primitive::attrib_offset), DefaultAlloc>
        attrib_offsets{get_default_alloc()};
    attrib_offsets.reserve(primitive_cnt);
    for (Mesh const& mesh : mesh_aggregate.meshes) {
        for (Mesh::Primitive const& primitive : mesh.primitives) {
            draw_cmds.push_back(VkDrawIndirectCommand{
                .vertexCount = (uint32_t) primitive.index_count,
                .instanceCount = 1,
                .firstVertex = (uint32_t) primitive.index_offset,
                .firstInstance = (uint32_t) draw_cmds.size(),
            });
            attrib_offsets.push_back(primitive.attrib_offset);
        }
    }
    m_draw_cmd_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) draw_cmds.data(), m_draw_cmd_buf.get_size()});
    m_attrib_offset_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{(const uint8_t*) attrib_offsets.data(),
            m_attrib_offset_buf.get_size()});
}

VulkanBuffer const& VulkanVertexIndexBuffer::get_vertex_buf() const noexcept {
    return m_vertex_buf;
}

VulkanBuffer const& VulkanVertexIndexBuffer::get_index_buf() const noexcept {
    return m_index_buf;
}

VulkanBuffer const& VulkanVertexIndexBuffer::get_draw_cmd_buf() const noexcept {
    return m_draw_cmd_buf;
}

VulkanBuffer const& VulkanVertexIndexBuffer::get_attrib_offset_buf()
    const noexcept {
    return m_attrib_offset_buf;
}

}  // namespace render
}  // namespace coust
