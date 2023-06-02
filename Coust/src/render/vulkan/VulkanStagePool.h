#pragma once

#include "utils/Compiler.h"
#include "core/Memory.h"
#include "utils/allocators/SmartPtr.h"
#include "utils/allocators/StlContainer.h"
#include "render/vulkan/utils/CacheSetting.h"
#include "render/vulkan/VulkanBuffer.h"
#include "render/vulkan/VulkanImage.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
#include "vk_mem_alloc.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanStagePool {
public:
    VulkanStagePool() = delete;
    VulkanStagePool(VulkanStagePool &&) = delete;
    VulkanStagePool(VulkanStagePool const &) = delete;
    VulkanStagePool &operator=(VulkanStagePool &&) = delete;
    VulkanStagePool &operator=(VulkanStagePool const &) = delete;

public:
    VulkanStagePool(VkDevice dev, VmaAllocator alloc) noexcept;

    memory::shared_ptr<VulkanBuffer> acquire_staging_buf(
        VkDeviceSize size) noexcept;

    memory::shared_ptr<VulkanHostImage> acquire_staging_img(
        VkCommandBuffer cmdbuf, VkFormat format, uint32_t width,
        uint32_t height) noexcept;

    void gc() noexcept;

    void reset() noexcept;

private:
    struct StagingBuffer {
        memory::shared_ptr<VulkanBuffer> buf;
        uint32_t last_accessed;
    };

    struct StagingImage {
        memory::shared_ptr<VulkanHostImage> img;
        uint32_t last_accessed;
    };

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VmaAllocator m_alloc = VK_NULL_HANDLE;

    memory::vector<StagingBuffer, DefaultAlloc> m_free_staging_bufs{
        get_default_alloc()};

    memory::vector<StagingBuffer, DefaultAlloc> m_used_staging_bufs{
        get_default_alloc()};

    memory::vector<StagingImage, DefaultAlloc> m_free_staging_imgs{
        get_default_alloc()};

    memory::vector<StagingImage, DefaultAlloc> m_used_staging_imgs{
        get_default_alloc()};

    GCTimer m_gc_timer;

    CacheHitCounter m_buffer_hit_counter;
    CacheHitCounter m_image_hit_counter;
};

}  // namespace render
}  // namespace coust
