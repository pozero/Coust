#include "pch.h"

#include "render/vulkan/VulkanMaterial.h"
#include "utils/Span.h"

namespace coust {
namespace render {

std::array<uint32_t, 3> constexpr related_queues{
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
};

VulkanMaterialBuffer::VulkanMaterialBuffer(VkDevice dev, VmaAllocator alloc,
    VkCommandBuffer cmdbuf, class VulkanStagePool& stage_pool,
    MeshAggregate const& mesh_aggregate) noexcept
    : m_material_index_buf(dev, alloc,
          sizeof(uint32_t) * MeshAggregate::get_primitve_count(mesh_aggregate),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues),
      m_material_buf(dev, alloc,
          sizeof(Material) * mesh_aggregate.material_aggregate.materials.size(),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          related_queues) {
    memory::vector<uint32_t, DefaultAlloc> material_indices{
        get_default_alloc()};
    material_indices.reserve(MeshAggregate::get_primitve_count(mesh_aggregate));
    for (auto const& mesh : mesh_aggregate.meshes) {
        for (auto const& primitive : mesh.primitives) {
            material_indices.push_back(primitive.material_index);
        }
    }
    m_material_index_buf.update(
        stage_pool, cmdbuf, to_span<const uint8_t>(material_indices));
    m_material_buf.update(stage_pool, cmdbuf,
        to_span<const uint8_t>(mesh_aggregate.material_aggregate.materials));
}

VulkanBuffer const& VulkanMaterialBuffer::get_material_index_buf()
    const noexcept {
    return m_material_index_buf;
}

VulkanBuffer const& VulkanMaterialBuffer::get_material_buf() const noexcept {
    return m_material_buf;
}

}  // namespace render
}  // namespace coust
