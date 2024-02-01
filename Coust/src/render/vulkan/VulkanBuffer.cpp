#include "pch.h"

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/PtrMath.h"
#include "render/vulkan/VulkanBuffer.h"
#include "render/vulkan/VulkanStagePool.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanTagger.h"

namespace coust {
namespace render {

constexpr void get_vma_allocation_config(VulkanBuffer::Usage usage,
    VkBufferUsageFlags& out_vk_buffer_usage, VmaMemoryUsage& out_mem_usage,
    VmaAllocationCreateFlags& out_allocation_flags) noexcept {
    switch (usage) {
        case VulkanBuffer::Usage::gpu_only:
            out_vk_buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            out_mem_usage = VMA_MEMORY_USAGE_AUTO;
            out_allocation_flags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
            return;
        case VulkanBuffer::Usage::staging:
            out_vk_buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            out_mem_usage = VMA_MEMORY_USAGE_AUTO;
            out_allocation_flags |=
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            return;
        case VulkanBuffer::Usage::read_back:
            out_vk_buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            out_mem_usage = VMA_MEMORY_USAGE_AUTO;
            out_allocation_flags |=
                VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            return;
        case VulkanBuffer::Usage::frequent_read_write:
            out_vk_buffer_usage |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
            out_vk_buffer_usage |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
            out_mem_usage = VMA_MEMORY_USAGE_AUTO;
            out_allocation_flags |=
                VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                VMA_ALLOCATION_CREATE_MAPPED_BIT;
            return;
    }
    ASSUME(0);
}

VkDevice VulkanBuffer::get_device() const noexcept {
    return m_dev;
}

VkBuffer VulkanBuffer::get_handle() const noexcept {
    return m_handle;
}

VulkanBuffer::VulkanBuffer(VkDevice dev, VmaAllocator alloc, VkDeviceSize size,
    VkBufferUsageFlags vk_buf_usage, Usage usage,
    std::array<uint32_t, 3> const& related_queues) noexcept
    : m_dev(dev), m_allocator(alloc), m_size(size) {
    VmaMemoryUsage vma_mem_usage = VMA_MEMORY_USAGE_AUTO;
    VmaAllocationCreateFlags vma_allocation_flags = 0;
    get_vma_allocation_config(
        usage, vk_buf_usage, vma_mem_usage, vma_allocation_flags);
    m_vk_usage = vk_buf_usage;
    VkBufferCreateInfo vk_buf_info{
        .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
        .flags = 0,
        .size = size,
        .usage = vk_buf_usage,
    };
    memory::vector<uint32_t, DefaultAlloc> all_queues{get_default_alloc()};
    std::ranges::copy_if(related_queues, std::back_inserter(all_queues),
        [](uint32_t const queue_idx) {
            return queue_idx != VK_QUEUE_FAMILY_IGNORED;
        });
    if (all_queues.size() > 1) {
        vk_buf_info.sharingMode = VK_SHARING_MODE_CONCURRENT;
        vk_buf_info.queueFamilyIndexCount = (uint32_t) all_queues.size();
        vk_buf_info.pQueueFamilyIndices = all_queues.data();
    }
    VmaAllocationCreateInfo vma_alloc_create_info{
        .flags = vma_allocation_flags,
        .usage = vma_mem_usage,
    };
    VmaAllocationInfo alloc_info{};
    COUST_VK_CHECK(
        vmaCreateBuffer(m_allocator, &vk_buf_info, &vma_alloc_create_info,
            &m_handle, &m_allocation, &alloc_info),
        "");
    VkMemoryPropertyFlags mem_props_flags;
    vmaGetAllocationMemoryProperties(
        m_allocator, m_allocation, &mem_props_flags);
    if (mem_props_flags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT &&
        mem_props_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        m_domain = MemoryDomain::host_device;
    else if (mem_props_flags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
        m_domain = MemoryDomain::host;
    else
        m_domain = MemoryDomain::device;
    if (mem_props_flags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
        m_update_mode = UpdateMode::auto_flush;
    if (usage == Usage::read_back)
        m_update_mode = UpdateMode::read_only;
    if (m_domain != MemoryDomain::device &&
        vma_allocation_flags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
        m_mapped = (uint8_t*) alloc_info.pMappedData;
}

VulkanBuffer::VulkanBuffer(VulkanBuffer&& other) noexcept
    : m_dev(other.m_dev),
      m_handle(other.m_handle),
      m_allocator(other.m_allocator),
      m_allocation(other.m_allocation),
      m_mapped(other.m_mapped),
      m_size(other.m_size),
      m_domain(other.m_domain),
      m_update_mode(other.m_update_mode),
      m_vk_usage(other.m_vk_usage) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
    other.m_allocator = VK_NULL_HANDLE;
    other.m_allocation = VK_NULL_HANDLE;
}

VulkanBuffer& VulkanBuffer::operator=(VulkanBuffer&& other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    std::swap(m_allocator, other.m_allocator);
    std::swap(m_allocation, other.m_allocation);
    std::swap(m_mapped, other.m_mapped);
    std::swap(m_size, other.m_size);
    std::swap(m_domain, other.m_domain);
    std::swap(m_update_mode, other.m_update_mode);
    std::swap(m_vk_usage, other.m_vk_usage);
    return *this;
}

VulkanBuffer::~VulkanBuffer() noexcept {
    if (m_allocation != VK_NULL_HANDLE) {
        vmaDestroyBuffer(m_allocator, m_handle, m_allocation);
    }
}

void VulkanBuffer::set_always_flush(bool should_always_flush) noexcept {
    if (m_update_mode == UpdateMode::auto_flush ||
        m_update_mode == UpdateMode::read_only)
        return;
    m_update_mode = should_always_flush ? UpdateMode::always_flush :
                                          UpdateMode::flush_on_demand;
}

void VulkanBuffer::flush() const noexcept {
    vmaFlushAllocation(m_allocator, m_allocation, 0, VK_WHOLE_SIZE);
}

void VulkanBuffer::update(class VulkanStagePool& stagePool,
    VkCommandBuffer cmdbuf, std::span<const uint8_t> data,
    size_t offset) noexcept {
    if (m_domain == MemoryDomain::device) {
        auto staging_buf = stagePool.acquire_staging_buf(data.size());
        staging_buf->update(stagePool, cmdbuf, data, offset);
        staging_buf->flush();
        VkBufferCopy copyInfo{
            .srcOffset = 0,
            .dstOffset = (uint32_t) offset,
            .size = data.size(),
        };
        vkCmdCopyBuffer(
            cmdbuf, staging_buf->get_handle(), m_handle, 1, &copyInfo);
        // Using memory barrier to make sure the data actually reaches the
        // memory before using it
        if (m_vk_usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ||
            m_vk_usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT) {
            VkBufferMemoryBarrier2 barrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                // wait for another possible upload
                .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT |
                                VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT |
                                 VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT |
                                 VK_ACCESS_2_INDEX_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            VkDependencyInfo dependency{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers = &barrier,
                .imageMemoryBarrierCount = 0,
                .pImageMemoryBarriers = nullptr,
            };
            vkCmdPipelineBarrier2(cmdbuf, &dependency);
        }
        if (m_vk_usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT) {
            VkBufferMemoryBarrier2 barrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                // wait for another possible upload
                .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT |
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT |
                                 VK_ACCESS_2_UNIFORM_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            VkDependencyInfo dependency{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                // .dependencyFlags;
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers = &barrier,
                .imageMemoryBarrierCount = 0,
                .pImageMemoryBarriers = nullptr,
            };
            vkCmdPipelineBarrier2(cmdbuf, &dependency);
        }
        if (m_vk_usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT) {
            VkBufferMemoryBarrier2 barrier{
                .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                // wait for another possible upload
                .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT |
                                VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT |
                                VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT |
                                 VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                .buffer = m_handle,
                .offset = 0,
                .size = VK_WHOLE_SIZE,
            };
            VkDependencyInfo dependency{
                .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                // .dependencyFlags;
                .memoryBarrierCount = 0,
                .pMemoryBarriers = nullptr,
                .bufferMemoryBarrierCount = 1,
                .pBufferMemoryBarriers = &barrier,
                .imageMemoryBarrierCount = 0,
                .pImageMemoryBarriers = nullptr,
            };
            vkCmdPipelineBarrier2(cmdbuf, &dependency);
        }
    } else {
        std::copy(data.begin(), data.end(), ptr_math::add(m_mapped, offset));
        if (m_update_mode == UpdateMode::always_flush)
            flush();
    }
}

std::span<const uint8_t> VulkanBuffer::get_mapped() const noexcept {
    if (m_update_mode == UpdateMode::read_only)
        return std::span<const uint8_t>{m_mapped, m_size};
    COUST_WARN("Trying to rand access a block of memory without "
               "`VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT`");
    return std::span<const uint8_t>{(const uint8_t*) nullptr, 0};
}

VkDeviceSize VulkanBuffer::get_size() const noexcept {
    return m_size;
}

static_assert(detail::IsVulkanResource<VulkanBuffer>);

}  // namespace render
}  // namespace coust
