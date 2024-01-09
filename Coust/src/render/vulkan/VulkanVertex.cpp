#include "pch.h"

#include "render/vulkan/VulkanVertex.h"
#include "utils/Span.h"

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
    : m_primitive_count(MeshAggregate::get_primitve_count(mesh_aggregate)),
      m_vertex_buf(dev, alloc,
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
          sizeof(VkDrawIndirectCommand) * m_primitive_count,
          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues),
      m_attrib_offset_buf(dev, alloc,
          sizeof(Mesh::Primitive::attrib_offset) * m_primitive_count,
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues) {
    m_vertex_buf.update(stage_pool, cmdbuf,
        to_span<const uint8_t>(mesh_aggregate.vertex_buffer));
    m_index_buf.update(stage_pool, cmdbuf,
        to_span<const uint8_t>(mesh_aggregate.index_buffer));
    size_t const primitive_cnt =
        MeshAggregate::get_primitve_count(mesh_aggregate);
    memory::vector<VkDrawIndirectCommand, DefaultAlloc> draw_cmds{
        get_default_alloc()};
    draw_cmds.reserve(primitive_cnt);
    memory::vector<decltype(Mesh::Primitive::attrib_offset), DefaultAlloc>
        attrib_offsets{get_default_alloc()};
    attrib_offsets.reserve(primitive_cnt);
    memory::vector<size_t, DefaultAlloc> mesh_draw_cmd_bytes_offset{
        get_default_alloc()};
    mesh_draw_cmd_bytes_offset.reserve(mesh_aggregate.meshes.size());
    for (Mesh const& mesh : mesh_aggregate.meshes) {
        mesh_draw_cmd_bytes_offset.push_back(
            draw_cmds.size() * sizeof(decltype(draw_cmds)::value_type));
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
    m_draw_cmd_buf.update(
        stage_pool, cmdbuf, to_span<const uint8_t>(draw_cmds));
    m_attrib_offset_buf.update(
        stage_pool, cmdbuf, to_span<const uint8_t>(attrib_offsets));
    for (uint32_t i = 0; i < mesh_aggregate.nodes.size(); ++i) {
        uint32_t mesh_idx = mesh_aggregate.nodes[i].mesh_idx;
        if (mesh_idx != (uint32_t) -1) {
            m_node_infos.push_back(NodeInfo{
                .draw_cmd_bytes_offset = mesh_draw_cmd_bytes_offset[mesh_idx],
                .primitive_count = (uint32_t) mesh_aggregate.meshes[mesh_idx]
                                       .primitives.size(),
                .node_idx = i,
            });
        }
    }
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

std::span<VulkanVertexIndexBuffer::NodeInfo const>
    VulkanVertexIndexBuffer::get_node_infos() const noexcept {
    return m_node_infos;
}

}  // namespace render
}  // namespace coust
