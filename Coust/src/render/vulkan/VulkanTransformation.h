#pragma once

#include "render/Mesh.h"
#include "render/vulkan/VulkanBuffer.h"

namespace coust {
namespace render {

class VulkanTransformationBuffer {
public:
    VulkanTransformationBuffer() = delete;
    VulkanTransformationBuffer(VulkanTransformationBuffer const&) = delete;
    VulkanTransformationBuffer& operator=(
        VulkanTransformationBuffer const&) = delete;

public:
    static std::string_view constexpr MAT_BUF_NAME{"MATRICES"};

    static std::string_view constexpr MAT_IDX_BUF_NAME{"MATRIX_INDICES"};

    static std::string_view constexpr MAT_IDX_IDX_NAME{"INDEX_INDICES"};

    static std::string_view constexpr DYNA_MAT_NAME{"DYNAMIC_MATRICES"};

    static std::string_view constexpr RES_MAT_NAME{"RESULT_MATRICES"};

public:
    VulkanTransformationBuffer(VkDevice dev, uint32_t graphics_queue_idx,
        uint32_t compute_queue_idx, VmaAllocator alloc, VkCommandBuffer cmdbuf,
        class VulkanStagePool& stage_pool,
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanTransformationBuffer(VulkanTransformationBuffer&&) noexcept = default;

    VulkanTransformationBuffer& operator=(
        VulkanTransformationBuffer&&) noexcept = default;

    void update_transformation(
        uint32_t node_idx, glm::mat4 const& mat) noexcept;

    VulkanBuffer const& get_matrices_buffer() const noexcept;

    VulkanBuffer const& get_matrix_indices_buffer() const noexcept;

    VulkanBuffer const& get_index_indices_buffer() const noexcept;

    VulkanBuffer const& get_result_matrices_buffer() const noexcept;

    VulkanBuffer const& get_dyna_mat_buf(
        VulkanStagePool& stage_pool, VkCommandBuffer cmdbuf) noexcept;

    std::tuple<uint32_t, uint32_t, uint32_t> get_work_group_size() const;

private:
    VulkanBuffer m_mat_buf;

    VulkanBuffer m_mat_idx_buf;

    VulkanBuffer m_idx_idx_buf;

    VulkanBuffer m_res_mat_buf;

    VulkanBuffer m_dyna_mat_buf;

    memory::vector<glm::mat4, DefaultAlloc> m_dyna_tran{get_default_alloc()};

    uint32_t m_node_count;
};

}  // namespace render
}  // namespace coust
