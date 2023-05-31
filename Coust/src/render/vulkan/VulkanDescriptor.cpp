#include "pch.h"

#include "utils/math/Hash.h"
#include "utils/Compiler.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/VulkanDescriptor.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wswitch")
constexpr VkDescriptorType get_descriptor_type(
    ShaderResourceType type, ShaderResourceUpdateMode mode) noexcept {
    switch (type) {
        case ShaderResourceType::input_attachment:
            return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        case ShaderResourceType::image:
            return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        case ShaderResourceType::sampler:
            return VK_DESCRIPTOR_TYPE_SAMPLER;
        case ShaderResourceType::image_sampler:
            return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        case ShaderResourceType::image_storage:
            return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        case ShaderResourceType::uniform_buffer:
            return mode == ShaderResourceUpdateMode::dyna ?
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ShaderResourceType::storage_buffer:
            return mode == ShaderResourceUpdateMode::dyna ?
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC :
                       VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    }
    ASSUME(0);
}
WARNING_POP

VkDevice VulkanDescriptorSetLayout::get_device() const noexcept {
    return m_dev;
}

VkDescriptorSetLayout VulkanDescriptorSetLayout::get_handle() const noexcept {
    return m_handle;
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice dev, uint32_t set,
    std::span<const VulkanShaderModule *const> shader_modules) noexcept
    : m_dev(dev), m_set(set) {
    memory::vector<VkDescriptorSetLayoutBinding, DefaultAlloc> vk_bindings{
        get_default_alloc()};
    for (auto const &shader : shader_modules) {
        hash_combine(m_hash, shader->get_byte_code_hash());
        for (auto const &res : shader->get_shader_resource()) {
            if (res.type == ShaderResourceType::input ||
                res.type == ShaderResourceType::output ||
                res.type == ShaderResourceType::push_constant ||
                res.type == ShaderResourceType::specialization_constant ||
                res.set != m_set) {
                continue;
            }
            COUST_PANIC_IF(m_idx_to_binding.contains(res.binding),
                "There are different shader resources with the same "
                "binding while constructing Vulkan Descriptor set layout. The "
                "conflicting binding is: Name {} -> Set {}, Binding {}",
                res.name, m_set, res.binding);
            VkDescriptorSetLayoutBinding binding{
                .binding = res.binding,
                .descriptorType =
                    get_descriptor_type(res.type, res.update_mode),
                .descriptorCount = res.array_size,
                .stageFlags = res.vk_shader_stage,
            };
            m_idx_to_binding.emplace(res.binding, binding);
            m_name_to_idx.emplace(res.name, res.binding);
            vk_bindings.push_back(binding);
        }
    }
    VkDescriptorSetLayoutCreateInfo desc_layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = 0,
        .bindingCount = (uint32_t) vk_bindings.size(),
        .pBindings = vk_bindings.data(),
    };
    COUST_VK_CHECK(vkCreateDescriptorSetLayout(m_dev, &desc_layout_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "Can't create vulkan descriptor set layout");
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkDescriptorSetLayoutBinding VulkanDescriptorSetLayout::get_binding(
    uint32_t binding) const noexcept {
    auto iter = m_idx_to_binding.find(binding);
    COUST_PANIC_IF(iter == m_idx_to_binding.end(),
        "Requesting non-existed binding to a vulkan descriptor set layout. The "
        "binding is: set {}, binding {}",
        m_set, binding);
    return iter->second;
}

uint32_t VulkanDescriptorSetLayout::get_set() const noexcept {
    return m_set;
}

size_t VulkanDescriptorSetLayout::get_hash() const noexcept {
    return m_hash;
}

auto VulkanDescriptorSetLayout::get_bindings() const noexcept
    -> decltype(m_idx_to_binding) const & {
    return m_idx_to_binding;
}

VkDevice VulkanDescriptorSet::get_device() const noexcept {
    return m_dev;
}

VkDescriptorSet VulkanDescriptorSet::get_handle() const noexcept {
    return m_handle;
}

bool VulkanDescriptorSet::Param::operator==(Param const &other) const noexcept {
    bool other_bol = set == other.set &&
                     allocator->get_layout().get_handle() ==
                         other.allocator->get_layout().get_handle() &&
                     buffer_infos.size() == other.buffer_infos.size() &&
                     image_infos.size() == other.image_infos.size();
    if (!other_bol)
        return false;
    for (uint32_t i = 0; i < buffer_infos.size(); ++i) {
        auto const &buf_arrl = buffer_infos[i];
        auto const &buf_arrr = other.buffer_infos[i];
        other_bol = buf_arrl.binding == buf_arrr.binding &&
                    buf_arrl.buffers.size() == buf_arrr.buffers.size();
        if (!other_bol)
            return false;
        for (uint32_t j = 0; j < buf_arrl.buffers.size(); ++j) {
            auto const &bl = buf_arrl.buffers[j];
            auto const &br = buf_arrr.buffers[j];
            other_bol = bl.buffer == br.buffer &&
                        bl.dst_array_idx == br.dst_array_idx &&
                        bl.offset == br.offset && bl.range == br.range;
            if (!other_bol)
                return false;
        }
    }
    for (uint32_t i = 0; i < image_infos.size(); ++i) {
        auto const &img_arrl = image_infos[i];
        auto const &img_arrr = other.image_infos[i];
        other_bol = img_arrl.binding == img_arrr.binding &&
                    img_arrl.images.size() == img_arrr.images.size();
        if (!other_bol)
            return false;
        for (uint32_t j = 0; j < img_arrl.images.size(); ++j) {
            auto const &il = img_arrl.images[j];
            auto const &ir = img_arrr.images[j];
            other_bol = il.image_view == ir.image_view &&
                        il.dst_array_idx == ir.dst_array_idx &&
                        il.image_layout == ir.image_layout &&
                        il.sampler == ir.sampler;
            if (!other_bol)
                return false;
        }
    }
    return true;
}

bool VulkanDescriptorSet::Param::operator!=(Param const &other) const noexcept {
    return !(*this == other);
}

VulkanDescriptorSet::VulkanDescriptorSet(
    VkDevice dev, VkPhysicalDevice phy_dev, Param const &param) noexcept
    : m_dev(dev),
      m_handle(param.allocator->allocate()),
      m_alloc(param.allocator),
      m_set(param.set) {
    VkPhysicalDeviceProperties phy_dev_props{};
    vkGetPhysicalDeviceProperties(phy_dev, &phy_dev_props);
    auto const &layout = param.allocator->get_layout();
    for (auto &buffer_array : param.buffer_infos) {
        if (buffer_array.binding == BoundBufferArray{}.binding) {
            continue;
        }
        uint32_t const binding = buffer_array.binding;
        auto const &buffers = buffer_array.buffers;
        VkDescriptorSetLayoutBinding bindingInfo = layout.get_binding(binding);
        for (auto &b : buffers) {
            if (b.buffer == VK_NULL_HANDLE)
                continue;

            // clamp the binding range to the GPU limit
            VkDeviceSize clampedRange = b.range;
            if (uint32_t uniformBufferRangeLimit =
                    phy_dev_props.limits.maxUniformBufferRange;
                (bindingInfo.descriptorType ==
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                    bindingInfo.descriptorType ==
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) &&
                clampedRange > uniformBufferRangeLimit) {
                COUST_WARN(
                    "The range (which is {}) of uniform buffer (Set {}, "
                    "Binding {}) to bind exceeds GPU limit (which is {})",
                    clampedRange, layout.get_set(), binding,
                    uniformBufferRangeLimit);
                clampedRange = uniformBufferRangeLimit;
            } else if (uint32_t storageBufferRangeLimit =
                           phy_dev_props.limits.maxStorageBufferRange;
                       (bindingInfo.descriptorType ==
                               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                           bindingInfo.descriptorType ==
                               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) &&
                       clampedRange > storageBufferRangeLimit) {
                COUST_WARN(
                    "The range (which is {}) of storage buffer (Set {}, "
                    "Binding {}) to bind exceeds GPU limit (which is {})",
                    clampedRange, layout.get_set(), binding,
                    storageBufferRangeLimit);
                clampedRange = storageBufferRangeLimit;
            }

            // https://en.cppreference.com/w/cpp/language/data_members#Standard-layout
            // A pointer to an object of standard-layout class type can be
            // reinterpret_cast to pointer to its first non-static non-bitfield
            // data member (if it has non-static data members) or otherwise any
            // of its base class subobjects (if it has any), and vice versa. In
            // other words, padding is not allowed before the first data member
            // of a standard-layout type.
            static_assert(std::is_standard_layout_v<BoundBuffer>, "");
            VkWriteDescriptorSet write{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_handle,
                .dstBinding = bindingInfo.binding,
                // record one element at a time
                .dstArrayElement = b.dst_array_idx,
                .descriptorCount = 1,
                .descriptorType = bindingInfo.descriptorType,
                // `BoundElement<*>` is a standard-layout class type so we can
                // convert its pointer directly to `VkDescriptorBufferInfo*`
                .pBufferInfo = (const VkDescriptorBufferInfo *) &b,
            };
            m_writes.push_back(write);
        }
    }

    for (auto &image_array : param.image_infos) {
        if (image_array.binding == BoundImageArray{}.binding)
            continue;
        uint32_t binding = image_array.binding;
        auto const &images = image_array.images;
        VkDescriptorSetLayoutBinding bindingInfo = layout.get_binding(binding);
        for (auto &i : images) {
            if (i.image_view == VK_NULL_HANDLE)
                continue;
            static_assert(std::is_standard_layout_v<BoundImage>, "");
            VkWriteDescriptorSet write{
                .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                .dstSet = m_handle,
                .dstBinding = bindingInfo.binding,
                // record one element at a time
                .dstArrayElement = i.dst_array_idx,
                .descriptorCount = 1,
                .descriptorType = bindingInfo.descriptorType,
                // `BoundElement<*>` is a standard-layout class type so we can
                // convert its pointer directly to `VkDescriptorBufferInfo*`
                .pImageInfo = (const VkDescriptorImageInfo *) &i,
            };
            m_writes.push_back(write);
        }
    }
}

VulkanDescriptorSet::~VulkanDescriptorSet() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        m_alloc->release(m_handle);
    }
}

void VulkanDescriptorSet::apply_write(
    std::span<const uint32_t> bindings_to_update) noexcept {
    memory::vector<VkWriteDescriptorSet, DefaultAlloc> write_not_applied{
        get_default_alloc()};
    write_not_applied.reserve(bindings_to_update.size());
    for (uint32_t const i : bindings_to_update) {
        if (!std::ranges::contains(m_applied_write_indices, i)) {
            write_not_applied.push_back(m_writes[i]);
            m_applied_write_indices.push_back(i);
        }
    }
    if (!write_not_applied.empty()) {
        vkUpdateDescriptorSets(m_dev, (uint32_t) write_not_applied.size(),
            write_not_applied.data(), 0, nullptr);
    }
}

void VulkanDescriptorSet::apply_write(bool overwrite) noexcept {
    if (overwrite) {
        vkUpdateDescriptorSets(
            m_dev, (uint32_t) m_writes.size(), m_writes.data(), 0, nullptr);
    } else {
        memory::vector<VkWriteDescriptorSet, DefaultAlloc> write_not_applied{
            get_default_alloc()};
        for (uint32_t i = 0; i < m_writes.size(); ++i) {
            if (!std::ranges::contains(m_applied_write_indices, i)) {
                write_not_applied.push_back(m_writes[i]);
                m_applied_write_indices.push_back(i);
            }
        }
        if (!write_not_applied.empty()) {
            vkUpdateDescriptorSets(m_dev, (uint32_t) write_not_applied.size(),
                write_not_applied.data(), 0, nullptr);
        }
    }
    m_applied_write_indices.clear();
    for (uint32_t i = 0; i < m_writes.size(); ++i) {
        m_applied_write_indices.push_back(i);
    }
}

uint32_t VulkanDescriptorSet::get_set() const noexcept {
    return m_set;
}

VulkanDescriptorSetAllocator::VulkanDescriptorSetAllocator(VkDevice dev,
    VulkanDescriptorSetLayout const &layout,
    uint32_t max_sets_per_pool) noexcept
    : m_dev(dev), m_layout(layout), m_max_set_per_pool(max_sets_per_pool) {
    auto const &bindings = layout.get_bindings();
    memory::robin_map<VkDescriptorType, uint32_t, DefaultAlloc>
        descriptor_type_cnts{get_default_alloc()};
    for (auto const &[idx, binding] : bindings) {
        if (descriptor_type_cnts.contains(binding.descriptorType)) {
            descriptor_type_cnts.at(binding.descriptorType) +=
                binding.descriptorCount;
        } else {
            descriptor_type_cnts.emplace(
                binding.descriptorType, binding.descriptorCount);
        }
    }
    m_pool_sizes.reserve(descriptor_type_cnts.size());
    std::transform(descriptor_type_cnts.begin(), descriptor_type_cnts.end(),
        std::back_inserter(m_pool_sizes), [max_sets_per_pool](auto const &p) {
            return VkDescriptorPoolSize{
                .type = p.first,
                .descriptorCount = p.second * max_sets_per_pool,
            };
        });
}

VulkanDescriptorSetAllocator::~VulkanDescriptorSetAllocator() noexcept {
    reset();
    for (auto const [pool, cnt] : m_pools) {
        vkDestroyDescriptorPool(m_dev, pool, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkDescriptorSet VulkanDescriptorSetAllocator::allocate() noexcept {
    if (!m_free_sets.empty()) {
        auto set = m_free_sets.back();
        m_free_sets.pop_back();
        return set;
    }
    find_pool();
    ++m_pools[m_pool_idx].second;
    VkDescriptorSetLayout layout = m_layout.get_handle();
    VkDescriptorSetAllocateInfo ai{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .descriptorPool = m_pools[m_pool_idx].first,
        .descriptorSetCount = 1,
        .pSetLayouts = &layout,
    };
    VkDescriptorSet set = VK_NULL_HANDLE;
    COUST_VK_CHECK(vkAllocateDescriptorSets(m_dev, &ai, &set),
        "Can't allocate vulkan descriptor set");
    return set;
}

void VulkanDescriptorSetAllocator::reset() noexcept {
    m_pool_idx = 0;
    for (auto &[pool, cnt] : m_pools) {
        cnt = 0;
        vkResetDescriptorPool(m_dev, pool, 0);
    }
    // descriptor sets have been implicitly freed by `vkResetDescriptorPool`
    m_free_sets.clear();
}

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wswitch-enum")
void VulkanDescriptorSetAllocator::fill_empty_descriptor_set_param(
    VulkanDescriptorSet::Param &param) noexcept {
    param.allocator = this;
    param.set = m_layout.get_set();
    uint32_t biggest_binding = 0;
    for (const auto &[idx, binding] : m_layout.get_bindings()) {
        biggest_binding = std::max(idx, biggest_binding);
    }
    // To avoid too much searching when binding resources later, we fill in all
    // possible bindings;
    param.buffer_infos.resize(biggest_binding + 1);
    param.image_infos.resize(biggest_binding + 1);
    for (const auto &[idx, binding] : m_layout.get_bindings()) {
        switch (binding.descriptorType) {
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                param.image_infos[idx].binding = idx;
                param.image_infos[idx].images.resize(binding.descriptorCount);
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                param.buffer_infos[idx].binding = idx;
                param.buffer_infos[idx].buffers.resize(binding.descriptorCount);
                break;
            default:
                break;
        }
    }
}
WARNING_POP

VulkanDescriptorSetLayout const &VulkanDescriptorSetAllocator::get_layout()
    const noexcept {
    return m_layout;
}

void VulkanDescriptorSetAllocator::find_pool() noexcept {
    uint32_t searchIdx = m_pool_idx;
    while (true) {
        // Searching reaches boundary, create new descriptor pool
        if (m_pools.size() <= searchIdx) {
            VkDescriptorPoolCreateInfo ci{
                .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                // We always reset the entire pool instead of freeing each
                // descriptor set individually And unlike command buffer which
                // is reset to "initial" status when the command buffer it
                // attached to is reset, resetting descriptor pool will
                // implicitly free all the descriptor sets attached to it.
                // .flags = m_layout->get_required_pool_flags(),
                .maxSets = m_max_set_per_pool,
                .poolSizeCount = (uint32_t) m_pool_sizes.size(),
                .pPoolSizes = m_pool_sizes.data(),
            };

            VkDescriptorPool pool = VK_NULL_HANDLE;
            COUST_VK_CHECK(vkCreateDescriptorPool(
                               m_dev, &ci, COUST_VULKAN_ALLOC_CALLBACK, &pool),
                "Can't create vulkan descriptor pool");

            m_pools.emplace_back(pool, 0);
            break;
        }
        // Or there's still enough capacity to allocate new descriptor set
        else if (m_pools[searchIdx].second < m_max_set_per_pool)
            break;
        // Capacity of current descriptor pool is depleted, search for next one
        else
            ++searchIdx;
    }
    m_pool_idx = searchIdx;
}

void VulkanDescriptorSetAllocator::release(VkDescriptorSet set) noexcept {
    m_free_sets.push_back(set);
}

static_assert(detail::IsVulkanResource<VulkanDescriptorSetLayout>);
static_assert(detail::IsVulkanResource<VulkanDescriptorSet>);

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<coust::render::BoundBuffer>::operator()(
    coust::render::BoundBuffer const &key) const noexcept {
    std::size_t h = coust::calc_std_hash(key.buffer);
    coust::hash_combine(h, key.offset);
    coust::hash_combine(h, key.range);
    coust::hash_combine(h, key.dst_array_idx);
    return h;
}

std::size_t hash<coust::render::BoundImage>::operator()(
    coust::render::BoundImage const &key) const noexcept {
    std::size_t h = coust::calc_std_hash(key.sampler);
    coust::hash_combine(h, key.image_view);
    coust::hash_combine(h, key.image_layout);
    coust::hash_combine(h, key.dst_array_idx);
    return h;
}

std::size_t hash<coust::render::VulkanDescriptorSet::Param>::operator()(
    coust::render::VulkanDescriptorSet::Param const &key) const noexcept {
    std::size_t h =
        coust::calc_std_hash(key.allocator->get_layout().get_hash());
    for (auto const &buffer_arr : key.buffer_infos) {
        for (auto const &buffer : buffer_arr.buffers) {
            coust::hash_combine(h, buffer);
        }
        coust::hash_combine(h, buffer_arr.binding);
    }
    for (auto const &image_arr : key.image_infos) {
        for (auto const &image : image_arr.images) {
            coust::hash_combine(h, image);
        }
        coust::hash_combine(h, image_arr.binding);
    }
    return h;
}

}  // namespace std
