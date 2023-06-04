#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "render/Mesh.h"
#include "render/vulkan/VulkanBuffer.h"

namespace coust {
namespace render {

class VulkanVertexBuffer {
public:
    VulkanVertexBuffer() = delete;
    VulkanVertexBuffer(VulkanVertexBuffer const&) = delete;
    VulkanVertexBuffer& operator=(VulkanVertexBuffer const&) = delete;

public:
    static std::string_view constexpr INDEX_BUF_NAME{"INDEX"};

    static std::string_view constexpr DRAW_CMD_BUF_NAME{"DRAW_CMD"};

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
    VulkanVertexBuffer(VkDevice dev, VmaAllocator alloc, VkCommandBuffer cmdbuf,
        class VulkanStagePool& stage_pool,
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanVertexBuffer(VulkanVertexBuffer&&) noexcept = default;

    VulkanVertexBuffer& operator=(VulkanVertexBuffer&&) noexcept = default;

    VulkanBuffer& get_vertex_buf() noexcept;

    VulkanBuffer& get_index_buf() noexcept;

    VulkanBuffer& get_draw_cmd_buf() noexcept;

    VulkanBuffer& get_attrib_offset_buf() noexcept;

private:
    memory::robin_map<VertexAttrib, VkDescriptorBufferInfo, DefaultAlloc>
        m_attrib_info{get_default_alloc()};

    VulkanBuffer m_vertex_buf;

    VulkanBuffer m_index_buf;

    VulkanBuffer m_draw_cmd_buf;

    VulkanBuffer m_attrib_offset_buf;
};

}  // namespace render
}  // namespace coust
