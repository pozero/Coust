#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
#include "vk_mem_alloc.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanBuffer {
public:
    VulkanBuffer() = delete;
    VulkanBuffer(VulkanBuffer const&) = delete;
    VulkanBuffer& operator=(VulkanBuffer const&) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_BUFFER;

    VkDevice get_device() const noexcept;

    VkBuffer get_handle() const noexcept;

    // The classification comes from the VMA official manual:
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
    enum class Usage {
        // Any resources that you frequently write and read on GPU,
        // e.g. images used as color attachments (aka "render targets"),
        // depth-stencil attachments, images/buffers used as storage
        // image/buffer (aka "Unordered Access View (UAV)").
        gpu_only,
        // A "staging" buffer than you want to map and fill from CPU code, then
        // use as a source of transfer to some GPU resource.
        staging,
        // Buffers for data written by or transferred from the GPU that you want
        // to read back on the CPU, e.g. results of some computations.
        read_back,
        // For resources that you frequently write on CPU via mapped pointer and
        // frequently read on GPU e.g. as a uniform buffer (also called
        // "dynamic") Be careful, frequent read write means it probably be
        // slower than the solution of copying data from staging buffer to a
        // device-local buffer. It needs to be measured.
        frequent_read_write,
    };

public:
    VulkanBuffer(VkDevice dev, VmaAllocator alloc, VkDeviceSize size,
        VkBufferUsageFlags vk_buf_usage, Usage usage,
        std::array<uint32_t, 3> const& related_queues) noexcept;

    VulkanBuffer(VulkanBuffer&&) noexcept = default;

    VulkanBuffer& operator=(VulkanBuffer&&) noexcept = default;

    ~VulkanBuffer() noexcept;

    void set_always_flush(bool should_always_flush) noexcept;

    // Flush memory if it's `HOST_VISIBLE` but not `HOST_COHERENT`
    // Note: Also, Windows drivers from all 3 PC GPU vendors (AMD, Intel,
    // NVIDIA) currently provide HOST_COHERENT flag on all memory types that are
    // HOST_VISIBLE, so on PC you may not need to bother.
    // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html#memory_mapping_persistently_mapped_memory
    void flush() const noexcept;

    void update(class StagePool* stagePool, VkCommandBuffer cmdbuf,
        std::span<const uint8_t> data, size_t offset = 0) noexcept;

    std::span<const uint8_t> get_mapped() const noexcept;

    VkDeviceSize get_size() const noexcept;

private:
    enum class MemoryDomain {
        host,
        device,
        // In both cases, the memory can be visible on both host side & device
        // side:
        //      1. On systems with unified memory (e.g. AMD APU or Intel
        //      integrated graphics, mobile chips),
        //         a memory type may be available that is both HOST_VISIBLE
        //         (available for mapping) and DEVICE_LOCAL (fast to access from
        //         the GPU).
        //      2. Systems with a discrete graphics card and separate video
        //      memory may or may not expose a memory type that is both
        //      HOST_VISIBLE and DEVICE_LOCAL,
        //         also known as Base Address Register (BAR).
        //         Writes performed by the host to that memory go through PCI
        //         Express bus. The performance of these writes may be limited,
        //         but it may be fine, especially on PCIe 4.0, as long as rules
        //         of using uncached and write-combined memory are followed -
        //         only sequential writes and no reads.
        host_device,
    };

    enum class UpdateMode {
        // If the buffer is used for read purpose
        read_only,
        // If the memory is `HOST_VISIBLE` but not `HOST_COHERENT`, then we can
        // choose from them to alleviate cache pressure strategically
        // The `Update()` function will always flush after copy
        always_flush,
        // The `Update()` func won't flush, it's the caller's responsiblity to
        // flush memory
        flush_on_demand,
        // If the memory is `HOST_COHERENT`, then there's no need to bother it
        auto_flush,
    };

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkBuffer m_handle = VK_NULL_HANDLE;

    VmaAllocator m_allocator = VK_NULL_HANDLE;

    VmaAllocation m_allocation = VK_NULL_HANDLE;

    uint8_t* m_mapped = nullptr;

    VkDeviceSize m_size;

    MemoryDomain m_domain;

    UpdateMode m_update_mode = UpdateMode::always_flush;

    VkBufferUsageFlags m_vk_usage;
};

}  // namespace render
}  // namespace coust
