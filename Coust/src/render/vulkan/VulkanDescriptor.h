#pragma once

#include "utils/Compiler.h"
#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "render/vulkan/VulkanShader.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

struct BoundBuffer {
    VkBuffer buffer = VK_NULL_HANDLE;
    VkDeviceSize offset = 0;
    VkDeviceSize range = 0;
    uint32_t dst_array_idx = ~(0u);

    auto operator<=>(BoundBuffer const &other) const noexcept = default;
};

struct BoundImage {
    VkSampler sampler = VK_NULL_HANDLE;
    VkImageView image_view = VK_NULL_HANDLE;
    VkImageLayout image_layout = VK_IMAGE_LAYOUT_UNDEFINED;
    uint32_t dst_array_idx = ~(0u);

    auto operator<=>(BoundImage const &other) const noexcept = default;
};

struct BoundBufferArray {
    memory::vector<BoundBuffer, DefaultAlloc> buffers{get_default_alloc()};
    uint32_t binding = ~(0u);
};

struct BoundImageArray {
    memory::vector<BoundImage, DefaultAlloc> images{get_default_alloc()};
    uint32_t binding = ~(0u);
};

class VulkanDescriptorSetLayout {
public:
    VulkanDescriptorSetLayout() = delete;
    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout const &) = delete;
    VulkanDescriptorSetLayout &operator=(VulkanDescriptorSetLayout &&) = delete;
    VulkanDescriptorSetLayout &operator=(
        VulkanDescriptorSetLayout const &) = delete;

public:
    static int constexpr object_type = VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT;

    VkDevice get_device() const noexcept;

    VkDescriptorSetLayout get_handle() const noexcept;

public:
    VulkanDescriptorSetLayout(VkDevice dev, VkPhysicalDevice phy_dev,
        uint32_t set,
        std::span<const VulkanShaderModule *const> shader_modules) noexcept;

    VulkanDescriptorSetLayout(VulkanDescriptorSetLayout &&other) noexcept;

    ~VulkanDescriptorSetLayout() noexcept;

    VkDescriptorType get_binding_type(uint32_t binding) const noexcept;

    uint32_t get_set() const noexcept;

    size_t get_hash() const noexcept;

    VkDescriptorPoolCreateFlags get_required_pool_flags() const noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkDescriptorSetLayout m_handle = VK_NULL_HANDLE;

    uint32_t m_set;

    memory::robin_map<uint32_t, VkDescriptorSetLayoutBinding, DefaultAlloc>
        m_idx_to_binding{get_default_alloc()};

    size_t m_hash = 0;

    VkDescriptorPoolCreateFlags m_required_pool_flags = 0;

public:
    auto get_bindings() const noexcept -> decltype(m_idx_to_binding) const &;
};

class VulkanDescriptorSet {
public:
    VulkanDescriptorSet() = delete;
    VulkanDescriptorSet(VulkanDescriptorSet const &) = delete;
    VulkanDescriptorSet &operator=(VulkanDescriptorSet const &) = delete;

public:
    static int constexpr object_type = VK_OBJECT_TYPE_DESCRIPTOR_SET;

    VkDevice get_device() const noexcept;

    VkDescriptorSet get_handle() const noexcept;

    struct Param {
        class VulkanDescriptorSetAllocator *allocator;
        memory::vector<BoundBufferArray, DefaultAlloc> buffer_infos{
            get_default_alloc()};
        memory::vector<BoundImageArray, DefaultAlloc> image_infos{
            get_default_alloc()};
        VkCommandBuffer attached_cmdbuf = VK_NULL_HANDLE;
        uint32_t set;
    };

public:
    VulkanDescriptorSet(
        VkDevice dev, VkPhysicalDevice phy_dev, Param const &param) noexcept;

    VulkanDescriptorSet(VulkanDescriptorSet &&other) noexcept;

    VulkanDescriptorSet &operator=(VulkanDescriptorSet &&other) noexcept;

    // Lifecycle of descriptor set is managed by descriptor set allocator, so
    // the descructor will release the handle back to its allocator.
    ~VulkanDescriptorSet() noexcept;

    void apply_write() const noexcept;

    uint32_t get_set() const noexcept;

private:
    struct WriteBufferInfo {
        uint32_t dstBinding;
        uint32_t dstArrayElement;
        VkDescriptorType descriptorType;

        VkBuffer buffer;
        VkDeviceSize offset;
        VkDeviceSize range;
    };

    struct WriteImageInfo {
        uint32_t dstBinding;
        uint32_t dstArrayElement;
        VkDescriptorType descriptorType;

        VkSampler sampler;
        VkImageView imageView;
        VkImageLayout imageLayout;
    };

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkDescriptorSet m_handle = VK_NULL_HANDLE;

    VulkanDescriptorSetAllocator *m_alloc = nullptr;

    memory::vector<WriteBufferInfo, DefaultAlloc> m_write_buf_infos{
        get_default_alloc()};

    memory::vector<WriteImageInfo, DefaultAlloc> m_write_img_infos{
        get_default_alloc()};

    uint32_t m_set;
};

class VulkanDescriptorSetAllocator {
public:
    VulkanDescriptorSetAllocator() = delete;
    VulkanDescriptorSetAllocator(VulkanDescriptorSetAllocator const &) = delete;
    VulkanDescriptorSetAllocator &operator=(
        VulkanDescriptorSetAllocator &&) = delete;
    VulkanDescriptorSetAllocator &operator=(
        VulkanDescriptorSetAllocator const &) = delete;

public:
    VulkanDescriptorSetAllocator(VkDevice dev,
        VulkanDescriptorSetLayout const &layout,
        uint32_t max_sets_per_pool = 16) noexcept;

    VulkanDescriptorSetAllocator(VulkanDescriptorSetAllocator &&other) noexcept;

    ~VulkanDescriptorSetAllocator() noexcept;

    // If we have free descriptor set returned by `DescriptorSet` then use it,
    // otherwise try to allocate a new one.
    VkDescriptorSet allocate() noexcept;

    // Reset the whole pools of descriptor pool, and also recycle all the
    // created descriptor set implicitly.
    void reset() noexcept;

    // Fill in empty buffer info and image info in construct parameter. It's
    // handy in constructing descriptor set.
    void fill_empty_descriptor_set_param(
        VulkanDescriptorSet::Param &param) noexcept;

    VulkanDescriptorSetLayout const &get_layout() const noexcept;

private:
    void find_pool() noexcept;

    friend VulkanDescriptorSet::~VulkanDescriptorSet() noexcept;

    void release(VkDescriptorSet set) noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VulkanDescriptorSetLayout const &m_layout;

    memory::vector<VkDescriptorPoolSize, DefaultAlloc> m_pool_sizes{
        get_default_alloc()};

    // { pool, sets count allocated from pool }
    memory::vector<std::pair<VkDescriptorPool, uint32_t>, DefaultAlloc> m_pools{
        get_default_alloc()};

    memory::vector<VkDescriptorSet, DefaultAlloc> m_free_sets{
        get_default_alloc()};

    uint32_t m_max_set_per_pool;

    uint32_t m_pool_idx = 0;
};

}  // namespace render
}  // namespace coust

namespace std {

template <>
struct hash<coust::render::BoundBuffer> {
    std::size_t operator()(
        coust::render::BoundBuffer const &key) const noexcept;
};

template <>
struct hash<coust::render::BoundImage> {
    std::size_t operator()(coust::render::BoundImage const &key) const noexcept;
};

template <>
struct hash<coust::render::VulkanDescriptorSet::Param> {
    std::size_t operator()(
        coust::render::VulkanDescriptorSet::Param const &key) const noexcept;
};

template <>
struct equal_to<coust::render::VulkanDescriptorSet::Param> {
    bool operator()(coust::render::VulkanDescriptorSet::Param const &left,
        coust::render::VulkanDescriptorSet::Param const &right) const noexcept;
};

}  // namespace std
