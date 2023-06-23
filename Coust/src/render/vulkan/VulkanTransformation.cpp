#include "pch.h"

#include "render/vulkan/VulkanTransformation.h"

namespace coust {
namespace render {

std::array<uint32_t, 3> constexpr exclusive_related_queues{
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
    VK_QUEUE_FAMILY_IGNORED,
};

VulkanTransformationBuffer::VulkanTransformationBuffer(VkDevice dev,
    uint32_t graphics_queue_idx, uint32_t compute_queue_idx, VmaAllocator alloc,
    VkCommandBuffer cmdbuf, class VulkanStagePool& stage_pool,
    MeshAggregate const& mesh_aggregate) noexcept
    : m_mat_buf(dev, alloc,
          mesh_aggregate.transformations.size() *
              sizeof(decltype(MeshAggregate::transformations)::value_type),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          exclusive_related_queues),
      m_mat_idx_buf(dev, alloc,
          MeshAggregate::get_transformation_index_count(mesh_aggregate) *
              sizeof(decltype(Node::transformation_indices)::value_type),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          exclusive_related_queues),
      m_idx_idx_buf(dev, alloc,
          (mesh_aggregate.nodes.size() + 2) * sizeof(uint32_t),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          exclusive_related_queues),
      m_res_mat_buf(dev, alloc, mesh_aggregate.nodes.size() * sizeof(glm::mat4),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, VulkanBuffer::Usage::gpu_only,
          {graphics_queue_idx, compute_queue_idx, VK_QUEUE_FAMILY_IGNORED}),
      m_dyna_mat_buf(dev, alloc,
          mesh_aggregate.nodes.size() * sizeof(glm::mat4),
          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
          VulkanBuffer::Usage::frequent_read_write, exclusive_related_queues),
      m_dyna_tran(
          mesh_aggregate.nodes.size(), glm::mat4{1.0f}, get_default_alloc()),
      m_node_count((uint32_t) mesh_aggregate.nodes.size()) {
    m_mat_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) mesh_aggregate.transformations.data(),
            m_mat_buf.get_size()});
    memory::vector<uint32_t, DefaultAlloc> mat_idx{get_default_alloc()};
    memory::vector<uint32_t, DefaultAlloc> idx_idx{get_default_alloc()};
    mat_idx.reserve(m_node_count);
    idx_idx.reserve(m_node_count + 2);
    idx_idx.push_back(m_node_count);
    for (Node const& node : mesh_aggregate.nodes) {
        idx_idx.push_back((uint32_t) mat_idx.size());
        std::copy(node.transformation_indices.begin(),
            node.transformation_indices.end(), std::back_inserter(mat_idx));
    }
    idx_idx.push_back((uint32_t) mat_idx.size());
    m_mat_idx_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) mat_idx.data(), m_mat_idx_buf.get_size()});
    m_idx_idx_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) idx_idx.data(), m_idx_idx_buf.get_size()});
}

void VulkanTransformationBuffer::update_transformation(
    uint32_t node_idx, glm::mat4 const& mat) noexcept {
    m_dyna_tran[node_idx] = mat;
}

VulkanBuffer const& VulkanTransformationBuffer::get_matrices_buffer()
    const noexcept {
    return m_mat_buf;
}

VulkanBuffer const& VulkanTransformationBuffer::get_matrix_indices_buffer()
    const noexcept {
    return m_mat_idx_buf;
}

VulkanBuffer const& VulkanTransformationBuffer::get_index_indices_buffer()
    const noexcept {
    return m_idx_idx_buf;
}

VulkanBuffer const& VulkanTransformationBuffer::get_result_matrices_buffer()
    const noexcept {
    return m_res_mat_buf;
}

VulkanBuffer const& VulkanTransformationBuffer::get_dyna_mat_buf(
    VulkanStagePool& stage_pool, VkCommandBuffer cmdbuf) noexcept {
    m_dyna_mat_buf.update(stage_pool, cmdbuf,
        std::span<const uint8_t>{
            (const uint8_t*) m_dyna_tran.data(), m_dyna_mat_buf.get_size()});
    return m_dyna_mat_buf;
}

uint32_t constexpr compute_shader_work_group_size = 8;

std::tuple<uint32_t, uint32_t, uint32_t>
    VulkanTransformationBuffer::get_work_group_size() const {
    float sqrt = std::sqrt((float) m_node_count);
    uint32_t s = (uint32_t) std::ceil(
        sqrt * (1.0f / (float) compute_shader_work_group_size));
    return std::make_tuple(s, s, 1);
}

}  // namespace render
}  // namespace coust
