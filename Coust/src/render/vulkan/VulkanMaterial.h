#pragma once

#include "render/Mesh.h"
#include "render/vulkan/VulkanBuffer.h"

namespace coust {
namespace render {

class VulkanMaterialBuffer {
public:
    VulkanMaterialBuffer() = delete;
    VulkanMaterialBuffer(VulkanMaterialBuffer const&) = delete;
    VulkanMaterialBuffer& operator=(VulkanMaterialBuffer const&) = delete;

public:
    static std::string_view constexpr MATERIAL_INDEX_NAME{"MATERIAL_INDEX"};

    static std::string_view constexpr MATERIAL_NAME{"MATERIALS"};

public:
    VulkanMaterialBuffer(VkDevice dev, VmaAllocator alloc,
        VkCommandBuffer cmdbuf, class VulkanStagePool& stage_pool,
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanMaterialBuffer(VulkanMaterialBuffer&&) noexcept = default;

    VulkanMaterialBuffer& operator=(VulkanMaterialBuffer&&) noexcept = default;

    VulkanBuffer const& get_material_index_buf() const noexcept;

    VulkanBuffer const& get_material_buf() const noexcept;

private:
    VulkanBuffer m_material_index_buf;

    VulkanBuffer m_material_buf;
};

}  // namespace render
}  // namespace coust
