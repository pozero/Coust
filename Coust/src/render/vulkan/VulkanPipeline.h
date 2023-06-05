#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/Compiler.h"
#include "render/vulkan/VulkanShader.h"
#include "render/vulkan/VulkanDescriptor.h"
#include "render/vulkan/VulkanRenderPass.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

class VulkanPipelineLayout {
public:
    VulkanPipelineLayout() = delete;
    VulkanPipelineLayout(VulkanPipelineLayout const &) = delete;
    VulkanPipelineLayout &operator=(VulkanPipelineLayout const &) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_PIPELINE_LAYOUT;

    VkDevice get_device() const noexcept;

    VkPipelineLayout get_handle() const noexcept;

    struct Param {
        std::span<const VulkanShaderModule *> shader_modules;
    };

public:
    VulkanPipelineLayout(VkDevice dev, Param const &param) noexcept;

    VulkanPipelineLayout(VulkanPipelineLayout &&) noexcept = default;

    VulkanPipelineLayout &operator=(VulkanPipelineLayout &&) noexcept = default;

    ~VulkanPipelineLayout() noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkPipelineLayout m_handle = VK_NULL_HANDLE;

    // A group of shader module corresponds to a  group of descriptor set
    // layouts, it doesn't make much sense to cache these layout because of this
    // multi to multi correlation. So here we let the pipeline layout MANAGES
    // these descriptor set layouts.
    memory::vector<VulkanDescriptorSetLayout, DefaultAlloc>
        m_descriptor_layouts{get_default_alloc()};

public:
    auto get_descriptor_set_layouts() const noexcept
        -> decltype(m_descriptor_layouts) const &;
};

// This class conveys per-pipeline specialization constant information, because
// there won't be much specialization constants durng creation. There's no need
// to store them per shader. So all the shader modules inside one pipeline will
// share the same constant id.
class SpecializationConstantInfo {
public:
    SpecializationConstantInfo() noexcept = default;
    SpecializationConstantInfo(
        const SpecializationConstantInfo &) noexcept = default;
    SpecializationConstantInfo &operator=(
        const SpecializationConstantInfo &) noexcept = default;
    SpecializationConstantInfo(
        SpecializationConstantInfo &&) noexcept = default;
    SpecializationConstantInfo &operator=(
        SpecializationConstantInfo &&) noexcept = default;

    template <typename T>
    bool add_constant(uint32_t id, const T &data) noexcept {
        bool const id_exists = std::ranges::any_of(
            m_entry, [id](VkSpecializationMapEntry const &entry) {
                return entry.constantID == id;
            });
        if (id_exists) {
            COUST_WARN(
                "The data for specialization constant of id {} already exists",
                id);
            return false;
        }
        m_entry.push_back(VkSpecializationMapEntry{
            .constantID = id,
            .offset = (uint32_t) m_data.size(),
            .size = sizeof(T),
        });
        std::copy(&data, &data + sizeof(data), std::back_inserter(m_data));
        return true;
    }

    template <typename T>
    bool chang_constant(uint32_t id, const T &data) noexcept {
        auto iter = std::ranges::find_if(
            m_entry, [id](VkSpecializationMapEntry const &entry) {
                return entry.constantID == id;
            });
        COUST_ASSERT(iter != m_entry.end(),
            "Vulkan specialization constant if id {} doesn't exist yet", id);
        COUST_ASSERT(iter->size == sizeof(T),
            "The size of previous data for specialization constant doesn't "
            "match the size of the new data");
        T *entry = (T *) (m_data.data() + iter->offset);
        *entry = data;
        return true;
    }

    VkSpecializationInfo get() const noexcept;

    size_t get_hash() const noexcept;

    bool operator==(SpecializationConstantInfo const &other) const noexcept;

    bool operator!=(SpecializationConstantInfo const &other) const noexcept;

private:
    memory::vector<VkSpecializationMapEntry, DefaultAlloc> m_entry{
        get_default_alloc()};
    memory::vector<uint8_t, DefaultAlloc> m_data{get_default_alloc()};
};

class VulkanGraphicsPipeline {
public:
    VulkanGraphicsPipeline() = delete;
    VulkanGraphicsPipeline(VulkanGraphicsPipeline const &) = delete;
    VulkanGraphicsPipeline &operator=(VulkanGraphicsPipeline const &) = delete;

public:
    static uint32_t constexpr object_type = VK_OBJECT_TYPE_PIPELINE;

    VkDevice get_device() const noexcept;

    VkPipeline get_handle() const noexcept;

    struct RasterState {
        // We don't use primitive restart since most of the time we will use the
        // triangle list, as the spec says: primitiveRestartEnable controls
        // whether a special vertex index value is treated as restarting the
        // assembly of primitives. This enable only applies to indexed draws
        // (vkCmdDrawIndexed, and vkCmdDrawIndexedIndirect), and the special
        // index value is either 0xFFFFFFFF when the indexType parameter of
        // vkCmdBindIndexBuffer is equal to VK_INDEX_TYPE_UINT32, or 0xFFFF when
        // indexType is equal to VK_INDEX_TYPE_UINT16. Primitive restart is not
        // allowed for list topologies. const
        // VkPipelineInputAssemblyStateCreateInfo* pInputAssemblyState;
        VkPrimitiveTopology topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        // We don't support tesselletion now
        // Viewport & scissor are dynamic states, their setting is postponed
        // until actual drawing We don't use depth clamp & rasterizer discard,
        // and lineWidth is always 1.0f
        VkPolygonMode polygon_mode = VK_POLYGON_MODE_FILL;
        VkCullModeFlags cull_mode = VK_CULL_MODE_BACK_BIT;
        VkFrontFace front_face = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        VkBool32 depth_bias_enable = VK_FALSE;
        float depth_bias_constant_factor = 0.0f;
        float depth_bias_clamp = 0.0f;
        float depth_bias_slope_factor = 0.0f;
        // We don't use sample shading or multisample coverage
        VkSampleCountFlagBits rasterization_samples = VK_SAMPLE_COUNT_1_BIT;
        // We don't use depth bounds test and stencil test, and the depth test
        // is always enabled
        VkBool32 depthWrite_enable = VK_TRUE;
        // https://github.com/KhronosGroup/Vulkan-Samples/pull/25
        // TODO: Use Reversed depth-buffer to get more even distribution of
        // precision
        VkCompareOp depth_compare_op = VK_COMPARE_OP_LESS_OR_EQUAL;
        // We don't use logical operation and constant blend value
        // Also, we use consistent color blend across all the render targets
        uint32_t color_target_count = 1;
        VkBool32 blend_enable = VK_FALSE;
        VkBlendFactor src_color_blend_factor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dst_color_blend_factor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp color_blend_op = VK_BLEND_OP_ADD;
        VkBlendFactor src_alpha_blend_factor = VK_BLEND_FACTOR_ONE;
        VkBlendFactor dst_alpha_blend_factor = VK_BLEND_FACTOR_ZERO;
        VkBlendOp alpha_blend_op = VK_BLEND_OP_ADD;
        VkColorComponentFlags color_write_mask =
            VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
            VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        // We use fixed dynamic state which contains viewport and scissor
    };

    struct Param {
        SpecializationConstantInfo special_const_info{};
        RasterState raster_state{};
        std::span<const VulkanShaderModule *const> shader_modules;
        const VulkanRenderPass *render_pass = nullptr;
        uint32_t subpass = ~(0u);
    };

public:
    VulkanGraphicsPipeline(VkDevice dev, VulkanPipelineLayout const &layout,
        VkPipelineCache cache, Param const &param) noexcept;

    VulkanGraphicsPipeline(VulkanGraphicsPipeline &&) noexcept = default;

    VulkanGraphicsPipeline &operator=(
        VulkanGraphicsPipeline &&) noexcept = default;

    ~VulkanGraphicsPipeline() noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    VkPipeline m_handle = VK_NULL_HANDLE;
};

}  // namespace render
}  // namespace coust

namespace std {

template <>
struct hash<coust::render::VulkanPipelineLayout::Param> {
    std::size_t operator()(
        coust::render::VulkanPipelineLayout::Param const &key) const noexcept;
};

template <>
struct hash<VkSpecializationMapEntry> {
    std::size_t operator()(VkSpecializationMapEntry const &key) const noexcept;
};

template <>
struct hash<coust::render::VulkanGraphicsPipeline::Param> {
    std::size_t operator()(
        coust::render::VulkanGraphicsPipeline::Param const &key) const noexcept;
};

template <>
struct equal_to<coust::render::VulkanPipelineLayout::Param> {
    bool operator()(coust::render::VulkanPipelineLayout::Param const &left,
        coust::render::VulkanPipelineLayout::Param const &right) const noexcept;
};

template <>
struct equal_to<coust::render::VulkanGraphicsPipeline::Param> {
    bool operator()(coust::render::VulkanGraphicsPipeline::Param const &left,
        coust::render::VulkanGraphicsPipeline::Param const &right)
        const noexcept;
};

}  // namespace std
