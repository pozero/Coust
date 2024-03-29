#include "pch.h"

#include "utils/math/Hash.h"
#include "utils/Compiler.h"
#include "render/Mesh.h"
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
            return mode == ShaderResourceUpdateMode::dynamic_update ?
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC :
                       VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        case ShaderResourceType::storage_buffer:
            return mode == ShaderResourceUpdateMode::dynamic_update ?
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

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(VkDevice dev,
    VkPhysicalDevice phy_dev, uint32_t set,
    std::span<const VulkanShaderModule *const> shader_modules) noexcept
    : m_dev(dev), m_set(set) {
    static VkPhysicalDeviceDescriptorIndexingProperties const
        desc_indexing_props = std::invoke([phy_dev] {
            VkPhysicalDeviceDescriptorIndexingProperties ret{
                .sType =
                    VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_PROPERTIES,
            };
            VkPhysicalDeviceProperties2 phydev_props{
                .sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2,
                .pNext = &ret,
            };
            vkGetPhysicalDeviceProperties2(phy_dev, &phydev_props);
            return ret;
        });
    VkDescriptorBindingFlags constexpr update_after_bind_flags =
        VK_DESCRIPTOR_BINDING_VARIABLE_DESCRIPTOR_COUNT_BIT_EXT |
        VK_DESCRIPTOR_BINDING_PARTIALLY_BOUND_BIT_EXT |
        VK_DESCRIPTOR_BINDING_UPDATE_AFTER_BIND_BIT_EXT |
        VK_DESCRIPTOR_BINDING_UPDATE_UNUSED_WHILE_PENDING_BIT_EXT;
    memory::vector<VkDescriptorSetLayoutBinding, DefaultAlloc> vk_bindings{
        get_default_alloc()};
    memory::vector<VkDescriptorBindingFlags, DefaultAlloc> binding_flags{
        get_default_alloc()};
    VkDescriptorSetLayoutCreateInfo desc_layout_info{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
        .flags = 0,
    };
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
            if (res.update_mode ==
                ShaderResourceUpdateMode::update_after_bind) {
                desc_layout_info.flags |=
                    VK_DESCRIPTOR_SET_LAYOUT_CREATE_UPDATE_AFTER_BIND_POOL_BIT;
                m_required_pool_flags =
                    VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
                binding_flags.push_back(update_after_bind_flags);
                COUST_ASSERT(res.type == ShaderResourceType::image_sampler,
                    "Didn't support descriptor indexing for type {}.",
                    (int) res.type);
                binding.descriptorCount =
                    desc_indexing_props
                        .maxDescriptorSetUpdateAfterBindSampledImages;
            } else {
                binding_flags.push_back(0);
            }
            m_idx_to_binding.emplace(res.binding, binding);
            vk_bindings.push_back(binding);
        }
    }
    VkDescriptorSetLayoutBindingFlagsCreateInfo desc_layout_binding_flags{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_BINDING_FLAGS_CREATE_INFO,
        .bindingCount = (uint32_t) binding_flags.size(),
        .pBindingFlags = binding_flags.data(),
    };
    desc_layout_info.pNext = &desc_layout_binding_flags;
    desc_layout_info.bindingCount = (uint32_t) vk_bindings.size();
    desc_layout_info.pBindings = vk_bindings.data();
    COUST_VK_CHECK(vkCreateDescriptorSetLayout(m_dev, &desc_layout_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "Can't create vulkan descriptor set layout");
}

VulkanDescriptorSetLayout::VulkanDescriptorSetLayout(
    VulkanDescriptorSetLayout &&other) noexcept
    : m_dev(other.m_dev),
      m_handle(other.m_handle),
      m_set(other.m_set),
      m_idx_to_binding(std::move(other.m_idx_to_binding)),
      m_hash(other.m_hash) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanDescriptorSetLayout::~VulkanDescriptorSetLayout() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(
            m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkDescriptorType VulkanDescriptorSetLayout::get_binding_type(
    uint32_t binding) const noexcept {
    auto iter = m_idx_to_binding.find(binding);
    COUST_PANIC_IF(iter == m_idx_to_binding.end(),
        "Requesting non-existed binding to a vulkan descriptor set layout. The "
        "binding is: set {}, binding {}",
        m_set, binding);
    return iter->second.descriptorType;
}

uint32_t VulkanDescriptorSetLayout::get_set() const noexcept {
    return m_set;
}

size_t VulkanDescriptorSetLayout::get_hash() const noexcept {
    return m_hash;
}

VkDescriptorPoolCreateFlags VulkanDescriptorSetLayout::get_required_pool_flags()
    const noexcept {
    return m_required_pool_flags;
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

VulkanDescriptorSet::VulkanDescriptorSet(
    VkDevice dev, VkPhysicalDevice phy_dev, Param const &param) noexcept
    : m_dev(dev),
      m_handle(param.allocator->allocate()),
      m_alloc(param.allocator),
      m_set(param.set) {
    COUST_ASSERT(param.attached_cmdbuf != VK_NULL_HANDLE, "");
    VkPhysicalDeviceProperties phy_dev_props{};
    vkGetPhysicalDeviceProperties(phy_dev, &phy_dev_props);
    auto const &layout = param.allocator->get_layout();
    for (auto &buffer_array : param.buffer_infos) {
        if (buffer_array.binding == BoundBufferArray{}.binding) {
            continue;
        }
        uint32_t const binding = buffer_array.binding;
        auto const &buffers = buffer_array.buffers;
        VkDescriptorType const binding_type = layout.get_binding_type(binding);
        for (auto &b : buffers) {
            if (b.buffer == VK_NULL_HANDLE)
                continue;
            if (uint32_t uniformBufferRangeLimit =
                    phy_dev_props.limits.maxUniformBufferRange;
                (binding_type == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                    binding_type ==
                        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) &&
                b.range > uniformBufferRangeLimit && b.range != VK_WHOLE_SIZE) {
                COUST_PANIC_IF(true,
                    "The range (which is {}) of uniform buffer (Set {}, "
                    "Binding {}) to bind exceeds GPU limit (which is {})",
                    b.range, layout.get_set(), binding,
                    uniformBufferRangeLimit);
            } else if (uint32_t storageBufferRangeLimit =
                           phy_dev_props.limits.maxStorageBufferRange;
                       (binding_type == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                           binding_type ==
                               VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) &&
                       b.range > storageBufferRangeLimit &&
                       b.range != VK_WHOLE_SIZE) {
                COUST_PANIC_IF(true,
                    "The range (which is {}) of storage buffer (Set {}, "
                    "Binding {}) to bind exceeds GPU limit (which is {})",
                    b.range, layout.get_set(), binding,
                    storageBufferRangeLimit);
            }
            WriteBufferInfo info{
                .dstBinding = binding,
                .dstArrayElement = b.dst_array_idx,
                .descriptorType = binding_type,
                .buffer = b.buffer,
                .offset = b.offset,
                .range = b.range,
            };
            m_write_buf_infos.push_back(info);
        }
    }
    for (auto &image_array : param.image_infos) {
        if (image_array.binding == BoundImageArray{}.binding)
            continue;
        uint32_t const binding = image_array.binding;
        auto const &images = image_array.images;
        VkDescriptorType const binding_type = layout.get_binding_type(binding);
        for (auto &i : images) {
            if (i.image_view == VK_NULL_HANDLE)
                continue;
            WriteImageInfo info{
                .dstBinding = binding,
                .dstArrayElement = i.dst_array_idx,
                .descriptorType = binding_type,
                .sampler = i.sampler,
                .imageView = i.image_view,
                .imageLayout = i.image_layout,
            };
            m_write_img_infos.push_back(info);
        }
    }
}

VulkanDescriptorSet::VulkanDescriptorSet(VulkanDescriptorSet &&other) noexcept
    : m_dev(other.m_dev),
      m_handle(other.m_handle),
      m_alloc(other.m_alloc),
      m_write_buf_infos(std::move(other.m_write_buf_infos)),
      m_write_img_infos(std::move(other.m_write_img_infos)),
      m_set(other.m_set) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanDescriptorSet &VulkanDescriptorSet::operator=(
    VulkanDescriptorSet &&other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    std::swap(m_alloc, other.m_alloc);
    std::swap(m_write_buf_infos, other.m_write_buf_infos);
    std::swap(m_write_img_infos, other.m_write_img_infos);
    std::swap(m_set, other.m_set);
    return *this;
}

VulkanDescriptorSet::~VulkanDescriptorSet() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        m_alloc->release(m_handle);
    }
}

void VulkanDescriptorSet::apply_write() const noexcept {
    memory::vector<VkWriteDescriptorSet, DefaultAlloc> writes{
        get_default_alloc()};
    writes.reserve(m_write_buf_infos.size() + m_write_img_infos.size());
    for (auto const &buf_info : m_write_buf_infos) {
        static_assert(std::is_standard_layout_v<WriteBufferInfo>);
        VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_handle,
            .dstBinding = buf_info.dstBinding,
            .dstArrayElement = buf_info.dstArrayElement,
            .descriptorCount = 1,
            .descriptorType = buf_info.descriptorType,
            .pBufferInfo = (const VkDescriptorBufferInfo *) &buf_info.buffer,
        };
        writes.push_back(write);
    }
    for (auto const &img_info : m_write_img_infos) {
        static_assert(std::is_standard_layout_v<WriteImageInfo>);
        VkWriteDescriptorSet write{
            .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
            .dstSet = m_handle,
            .dstBinding = img_info.dstBinding,
            .dstArrayElement = img_info.dstArrayElement,
            .descriptorCount = 1,
            .descriptorType = img_info.descriptorType,
            .pImageInfo = (const VkDescriptorImageInfo *) &img_info.sampler,
        };
        writes.push_back(write);
    }
    vkUpdateDescriptorSets(
        m_dev, (uint32_t) writes.size(), writes.data(), 0, nullptr);
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

VulkanDescriptorSetAllocator::VulkanDescriptorSetAllocator(
    VulkanDescriptorSetAllocator &&other) noexcept
    : m_dev(other.m_dev),
      m_layout(other.m_layout),
      m_pool_sizes(std::move(other.m_pool_sizes)),
      m_pools(std::move(other.m_pools)),
      m_free_sets(std::move(other.m_free_sets)),
      m_max_set_per_pool(other.m_max_set_per_pool),
      m_pool_idx(other.m_pool_idx) {
    other.m_dev = VK_NULL_HANDLE;
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
    VkDescriptorSetVariableDescriptorCountAllocateInfo variable_desc_cnt_ai{
        .sType =
            VK_STRUCTURE_TYPE_DESCRIPTOR_SET_VARIABLE_DESCRIPTOR_COUNT_ALLOCATE_INFO,
        .descriptorSetCount = m_layout.get_required_pool_flags() == 0 ? 0 : 1,
        .pDescriptorCounts = &MAX_TEXTURE_PER_MESH_AGGREGATE,
    };
    VkDescriptorSetLayout layout = m_layout.get_handle();
    VkDescriptorSetAllocateInfo ai{
        .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
        .pNext = &variable_desc_cnt_ai,
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
                .flags = m_layout.get_required_pool_flags(),
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
    std::size_t h = key.allocator->get_layout().get_hash();
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
    coust::hash_combine(h, key.attached_cmdbuf);
    coust::hash_combine(h, key.set);
    return h;
}

bool equal_to<coust::render::VulkanDescriptorSet::Param>::operator()(
    coust::render::VulkanDescriptorSet::Param const &left,
    coust::render::VulkanDescriptorSet::Param const &right) const noexcept {
    bool other_bol = left.set == right.set &&
                     left.attached_cmdbuf == right.attached_cmdbuf &&
                     left.allocator->get_layout().get_handle() ==
                         right.allocator->get_layout().get_handle();
    if (!other_bol)
        return false;
    return std::ranges::equal(left.buffer_infos, right.buffer_infos,
               [](coust::render::BoundBufferArray const &l,
                   coust::render::BoundBufferArray const &r) {
                   return l.binding == r.binding &&
                          std::ranges::equal(l.buffers, r.buffers);
               }) &&
           std::ranges::equal(left.image_infos, right.image_infos,
               [](coust::render::BoundImageArray const &l,
                   coust::render::BoundImageArray const &r) {
                   return l.binding == r.binding &&
                          std::ranges::equal(l.images, r.images);
               });
}

}  // namespace std
