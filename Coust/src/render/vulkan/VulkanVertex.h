#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "render/Mesh.h"
#include "render/vulkan/VulkanBuffer.h"

namespace coust {
namespace render {

class VulkanVertexIndexBuffer {
public:
    VulkanVertexIndexBuffer() = delete;
    VulkanVertexIndexBuffer(VulkanVertexIndexBuffer const&) = delete;
    VulkanVertexIndexBuffer& operator=(VulkanVertexIndexBuffer const&) = delete;

public:
    static std::string_view constexpr INDEX_BUF_NAME{"INDEX"};

    static std::string_view constexpr ATTRIB_OFFSET_BUF_NAME{"ATTRIB_OFFSET"};

    /*
    typedef struct VkDrawIndirectCommand {
        uint32_t    vertexCount;        - VertexIndex
        uint32_t    instanceCount;      - InstanceIndex
        uint32_t    firstVertex;        - BaseVertex
        uint32_t    firstInstance;      - BaseInstance
    } VkDrawIndirectCommand;
    vkCmdDrawIndirect(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer,
        VkDeviceSize offset,
        uint32_t drawCount,             - DrawIndex
        uint32_t stride
    );
    attrib[ai][primitive.attrib_offset[ai] +
        mesh_aggregate.index_buffer[primitive.index_offset + cur_idx]]
    */

public:
    VulkanVertexIndexBuffer(VkDevice dev, VmaAllocator alloc,
        VkCommandBuffer cmdbuf, class VulkanStagePool& stage_pool,
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanVertexIndexBuffer(VulkanVertexIndexBuffer&&) noexcept = default;

    VulkanVertexIndexBuffer& operator=(
        VulkanVertexIndexBuffer&&) noexcept = default;

    VulkanBuffer const& get_vertex_buf() const noexcept;

    VulkanBuffer const& get_index_buf() const noexcept;

    VulkanBuffer const& get_draw_cmd_buf() const noexcept;

    VulkanBuffer const& get_attrib_offset_buf() const noexcept;

    size_t get_primitive_count() const noexcept;

private:
    size_t m_primitive_count;

    memory::robin_map<VertexAttrib, VkDescriptorBufferInfo, DefaultAlloc>
        m_attrib_info{get_default_alloc()};

    VulkanBuffer m_vertex_buf;

    VulkanBuffer m_index_buf;

    VulkanBuffer m_draw_cmd_buf;

    VulkanBuffer m_attrib_offset_buf;

public:
    auto get_attrib_infos() const noexcept -> decltype(m_attrib_info) const&;
};

}  // namespace render
}  // namespace coust
