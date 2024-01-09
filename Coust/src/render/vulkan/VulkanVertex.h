#pragma once

#include "core/Memory.h"
#include "render/Mesh.h"
#include "render/vulkan/VulkanBuffer.h"
#include "utils/allocators/StlContainer.h"

namespace coust {
namespace render {

class VulkanVertexIndexBuffer {
public:
    VulkanVertexIndexBuffer() = delete;
    VulkanVertexIndexBuffer(VulkanVertexIndexBuffer const&) = delete;
    VulkanVertexIndexBuffer& operator=(VulkanVertexIndexBuffer const&) = delete;

public:
    static std::string_view constexpr VERTEX_BUF_NAME{"VERTEX"};

    static std::string_view constexpr INDEX_BUF_NAME{"INDEX"};

    static std::string_view constexpr ATTRIB_OFFSET_BUF_NAME{"ATTRIB_OFFSET"};

    struct NodeInfo {
        size_t draw_cmd_bytes_offset;
        uint32_t primitive_count;
        uint32_t node_idx;
    };

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

    std::span<NodeInfo const> get_node_infos() const noexcept;

private:
    size_t m_primitive_count;

    memory::vector<NodeInfo, DefaultAlloc> m_node_infos{get_default_alloc()};

    VulkanBuffer m_vertex_buf;

    VulkanBuffer m_index_buf;

    VulkanBuffer m_draw_cmd_buf;

    VulkanBuffer m_attrib_offset_buf;
};

}  // namespace render
}  // namespace coust
