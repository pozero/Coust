#pragma once

#include "render/vulkan/VulkanRenderTarget.h"
#include "utils/Compiler.h"
#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/AlignedStorage.h"
#include "render/vulkan/VulkanCommand.h"
#include "render/vulkan/VulkanPipelineCache.h"
#include "render/vulkan/VulkanFBOCache.h"
#include "render/vulkan/VulkanSampler.h"
#include "render/vulkan/VulkanStagePool.h"
#include "render/vulkan/VulkanSwapchain.h"
#include "render/vulkan/VulkanVertex.h"
#include "render/vulkan/VulkanTransformation.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

VK_DEFINE_HANDLE(VmaAllocator)

namespace coust {
namespace render {

class VulkanDriver {
public:
    VulkanDriver() = delete;
    VulkanDriver(VulkanDriver&&) = delete;
    VulkanDriver(VulkanDriver const&) = delete;
    VulkanDriver& operator=(VulkanDriver&&) = delete;
    VulkanDriver& operator=(VulkanDriver const&) = delete;

public:
    VulkanDriver(memory::vector<const char*, DefaultAlloc>& instance_extension,
        memory::vector<const char*, DefaultAlloc>& instance_layer,
        memory::vector<const char*, DefaultAlloc>& device_extension,
        VkPhysicalDeviceFeatures const& required_phydev_feature,
        const void* instance_creation_pnext,
        const void* device_creation_pnext) noexcept;

    ~VulkanDriver() noexcept;

    void wait() noexcept;

    void gc() noexcept;

    void begin_frame() noexcept;

    void end_frame() noexcept;

    VkSampler create_sampler(VulkanSamplerParam const& param) noexcept;

    VulkanBuffer create_buffer_single_queue(VkDeviceSize size,
        VkBufferUsageFlags vk_buf_usage, VulkanBuffer::Usage usage) noexcept;

    VulkanVertexIndexBuffer create_vertex_index_buffer(
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanTransformationBuffer create_transformation_buffer(
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanImage create_image_single_queue(uint32_t width, uint32_t height,
        uint32_t levels, VkSampleCountFlagBits samples, VkFormat format,
        VulkanImage::Usage usage) noexcept;

    VulkanRenderTarget create_render_target(uint32_t width, uint32_t height,
        VkSampleCountFlagBits samples,
        std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> const& colors,
        VulkanAttachment const& depth) noexcept;

    VulkanRenderTarget const& get_attached_render_target() const noexcept;

    void update_buffer(VulkanBuffer& buffer, std::span<const uint8_t> data,
        size_t offset = 0) noexcept;

    void update_image(VulkanImage& image, VkFormat format, uint32_t width,
        uint32_t height, const void* data, uint32_t dst_level,
        uint32_t dst_layer = 0, uint32_t dst_layer_cnt = 1) noexcept;

    void begin_render_pass(VulkanRenderTarget const& render_target,
        AttachmentFlags clear_mask, AttachmentFlags discard_start_mask,
        AttachmentFlags discard_end_mask, uint8_t input_attachment_mask,
        VkClearValue clear_val) noexcept;

    void next_subpass() noexcept;

    void end_render_pass() noexcept;

    void update_swapchain() noexcept;

    void graphics_commit_present() noexcept;

    void commit_compute() noexcept;

    SpecializationConstantInfo& bind_specialization_info(
        VkPipelineBindPoint bind_point) noexcept;

    void bind_shader(VkPipelineBindPoint bind_point,
        VkShaderStageFlagBits vk_shader_stage,
        std::filesystem::path source_path) noexcept;

    void bind_buffer_whole(VkPipelineBindPoint bind_point,
        std::string_view name, VulkanBuffer const& buffer,
        uint32_t array_idx = 0) noexcept;

    void bind_buffer(VkPipelineBindPoint bind_point, std::string_view name,
        VulkanBuffer const& buffer, uint64_t offset, uint64_t size,
        uint32_t array_idx = 0) noexcept;

    void bind_image(VkPipelineBindPoint bind_point, std::string_view name,
        VkSampler sampler, VulkanImage const& image,
        uint32_t array_idx = 0) noexcept;

    void calculate_transformation(
        VulkanTransformationBuffer& transformation_buf) noexcept;

    void draw(VulkanVertexIndexBuffer const& vertex_index_buf,
        VulkanTransformationBuffer const& transformation_buf,
        VulkanGraphicsPipeline::RasterState const& raster_state,
        VkRect2D scissor) noexcept;

    void refresh_swapchain() noexcept;

    void add_compute_to_graphics_dependency() noexcept;

private:
    int32_t m_max_msaa_sample = 1;
    uint32_t m_graphics_queue_family_idx = std::numeric_limits<uint32_t>::max();
    uint32_t m_present_queue_family_idx = std::numeric_limits<uint32_t>::max();
    uint32_t m_compute_queue_family_idx = std::numeric_limits<uint32_t>::max();

private:
    VkInstance m_instance = VK_NULL_HANDLE;
#if defined(COUST_VK_DBG)
    VkDebugUtilsMessengerEXT m_dbg_messenger = VK_NULL_HANDLE;
#endif
    VkSurfaceKHR m_surface = VK_NULL_HANDLE;
    VkPhysicalDevice m_phydev = VK_NULL_HANDLE;
    VkDevice m_dev = VK_NULL_HANDLE;
    VkQueue m_graphics_queue = VK_NULL_HANDLE;
    VkQueue m_present_queue = VK_NULL_HANDLE;
    VkQueue m_compute_queue = VK_NULL_HANDLE;
    VmaAllocator m_vma_alloc = VK_NULL_HANDLE;

private:
    AlignedStorage<VulkanCommandBufferCache> m_graphics_cmdbuf_cache{};

    AlignedStorage<VulkanCommandBufferCache> m_compute_cmdbuf_cache{};

    AlignedStorage<VulkanShaderPool> m_shader_pool{};

    AlignedStorage<VulkanDescriptorCache> m_descriptor_cache{};

    AlignedStorage<VulkanGraphicsPipelineCache> m_graphics_pipeline_cache{};

    AlignedStorage<VulkanComputePipelineCache> m_compute_pipeline_cache{};

    AlignedStorage<VulkanFBOCache> m_fbo_cache{};

    AlignedStorage<VulkanSamplerCache> m_sampler_cache{};

    AlignedStorage<VulkanStagePool> m_stage_pool{};

    AlignedStorage<VulkanSwapchain> m_swapchain{};

private:
    VulkanRenderTarget m_attached_render_target{};

    const VulkanRenderTarget* m_cur_render_target = nullptr;

    const VulkanRenderPass* m_cur_render_pass = nullptr;

    uint32_t m_cur_subpass = 0;

    uint8_t m_cur_subpass_mask = 0;
};

}  // namespace render
}  // namespace coust
