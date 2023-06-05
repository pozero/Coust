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

    void gc() noexcept;

    void begin_frame() noexcept;

    void end_frame() noexcept;

    VkSampler create_sampler(VulkanSamplerParam const& param) noexcept;

    VulkanBuffer create_buffer_single_queue(VkDeviceSize size,
        VkBufferUsageFlags vk_buf_usage, VulkanBuffer::Usage usage) noexcept;

    VulkanVertexIndexBuffer create_vertex_index_buffer(
        MeshAggregate const& mesh_aggregate) noexcept;

    VulkanImage create_image_single_queue(uint32_t width, uint32_t height,
        uint32_t levels, uint32_t samples, VkFormat format,
        VulkanImage::Usage usage) noexcept;

    VulkanRenderTarget create_render_target(uint32_t width, uint32_t height,
        uint32_t samples,
        std::array<VulkanAttachment, MAX_ATTACHMENT_COUNT> const& colors,
        VulkanAttachment const& depth) noexcept;

    void update_buffer(
        std::span<const uint8_t> data, size_t offset = 0) noexcept;

    void update_image(VkFormat format, uint32_t width, uint32_t height,
        const void* data, uint32_t dst_level, uint32_t dst_layer = 0,
        uint32_t dst_layer_cnt = 1) noexcept;

    void begin_render_pass(VulkanRenderTarget const& render_target,
        VulkanRenderPass::Param const& param) noexcept;

    void next_subpass() noexcept;

    void end_render_pass() noexcept;

    void update_swapchain() noexcept;

    void commit() noexcept;

    SpecializationConstantInfo& bind_specialization_info() noexcept;

    void bind_shader(VkShaderStageFlagBits vk_shader_stage,
        std::filesystem::path source_path,
        std::span<std::string_view> dynamic_buffer_names) noexcept;

    void bind_buffer(std::string_view name, VulkanBuffer const& buffer,
        uint64_t offset, uint64_t size, uint32_t arrayIdx) noexcept;

    void bind_image(std::string_view name, VkSampler sampler,
        VulkanImage const& image, uint32_t arrayIdx) noexcept;

    void draw(VulkanVertexIndexBuffer const& vertex_index_buf,
        VulkanGraphicsPipeline::RasterState const& raster_state,
        VkRect2D scissor) noexcept;

    void refresh_swapchain() noexcept;

public:
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
    AlignedStorage<VulkanCommandBufferCache> m_cmdbuf_cache{};

    AlignedStorage<VulkanGraphicsPipelineCache> m_graphics_pipeline_cache{};

    AlignedStorage<VulkanFBOCache> m_fbo_cache{};

    AlignedStorage<VulkanSamplerCache> m_sampler_cache{};

    AlignedStorage<VulkanStagePool> m_stage_pool{};

    AlignedStorage<VulkanSwapchain> m_swapchain{};
};

}  // namespace render
}  // namespace coust
