#include "pch.h"

#include "render/vulkan/utils/VulkanCheck.h"
#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/utils/VulkanAllocation.h"
#include "render/vulkan/VulkanPipeline.h"
#include "utils/math/Hash.h"

namespace coust {
namespace render {

VkDevice VulkanPipelineLayout::get_device() const noexcept {
    return m_dev;
}

VkPipelineLayout VulkanPipelineLayout::get_handle() const noexcept {
    return m_handle;
}

VulkanPipelineLayout::VulkanPipelineLayout(
    VkDevice dev, Param const& param) noexcept
    : m_dev(dev) {
    memory::robin_set<uint32_t, DefaultAlloc> all_sets{get_default_alloc()};
    memory::vector<VkPushConstantRange, DefaultAlloc> push_constant_ranges{
        get_default_alloc()};
    for (auto const shader : param.shader_modules) {
        for (auto const& res : shader->get_shader_resource()) {
            all_sets.insert(res.set);
            if (res.type == ShaderResourceType::push_constant) {
                push_constant_ranges.push_back(VkPushConstantRange{
                    .stageFlags = res.vk_shader_stage,
                    .offset = res.offset,
                    .size = res.size,
                });
            }
        }
    }
    for (auto const set : all_sets) {
        m_descriptor_layouts.emplace_back(dev, set,
            std::span<const VulkanShaderModule* const>{
                param.shader_modules.data(), param.shader_modules.size()});
    }
    memory::vector<VkDescriptorSetLayout, DefaultAlloc> layout_handles{
        get_default_alloc()};
    layout_handles.reserve(m_descriptor_layouts.size());
    for (auto const& layout : m_descriptor_layouts) {
        layout_handles.push_back(layout.get_handle());
    }
    VkPipelineLayoutCreateInfo const pipeline_layout_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
        .setLayoutCount = (uint32_t) layout_handles.size(),
        .pSetLayouts = layout_handles.data(),
        .pushConstantRangeCount = (uint32_t) push_constant_ranges.size(),
        .pPushConstantRanges = push_constant_ranges.data(),
    };
    COUST_VK_CHECK(vkCreatePipelineLayout(m_dev, &pipeline_layout_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "Can't create vulkan pipeline layout");
}

VulkanPipelineLayout::VulkanPipelineLayout(
    VulkanPipelineLayout&& other) noexcept
    : m_dev(other.m_dev),
      m_handle(other.m_handle),
      m_descriptor_layouts(std::move(other.m_descriptor_layouts)) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanPipelineLayout& VulkanPipelineLayout::operator=(
    VulkanPipelineLayout&& other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    std::swap(m_descriptor_layouts, other.m_descriptor_layouts);
    return *this;
}

VulkanPipelineLayout::~VulkanPipelineLayout() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

auto VulkanPipelineLayout::get_descriptor_set_layouts() const noexcept
    -> decltype(m_descriptor_layouts) const& {
    return m_descriptor_layouts;
}

VkSpecializationInfo SpecializationConstantInfo::get() const noexcept {
    return VkSpecializationInfo{
        .mapEntryCount = (uint32_t) m_entry.size(),
        .pMapEntries = m_entry.data(),
        .dataSize = (uint32_t) m_data.size(),
        .pData = m_data.data(),
    };
}

size_t SpecializationConstantInfo::get_hash() const noexcept {
    size_t h = 0;
    for (auto const& entry : m_entry) {
        hash_combine(h, entry);
    }
    for (auto const& byte : m_data) {
        hash_combine(h, byte);
    }
    return h;
}

bool SpecializationConstantInfo::operator==(
    SpecializationConstantInfo const& other) const noexcept {
    bool other_bol = m_entry.size() == other.m_entry.size() &&
                     m_data.size() == other.m_data.size();
    if (!other_bol)
        return false;
    return std::ranges::equal(m_data, other.m_data) &&
           std::ranges::equal(m_entry, other.m_entry,
               [](VkSpecializationMapEntry const& left,
                   VkSpecializationMapEntry const& right) {
                   return left.constantID == right.constantID &&
                          left.offset == right.offset &&
                          left.size == right.size;
               });
}

bool SpecializationConstantInfo::operator!=(
    SpecializationConstantInfo const& other) const noexcept {
    return !(*this == other);
}

VkDevice VulkanGraphicsPipeline::get_device() const noexcept {
    return m_dev;
}

VkPipeline VulkanGraphicsPipeline::get_handle() const noexcept {
    return m_handle;
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(VkDevice dev,
    VulkanPipelineLayout const& layout, VkPipelineCache cache,
    Param const& param) noexcept
    : m_dev(dev) {
    VkSpecializationInfo const special_const_info =
        param.special_const_info.get();
    memory::vector<VkPipelineShaderStageCreateInfo, DefaultAlloc> stage_info{
        get_default_alloc()};
    stage_info.reserve(param.shader_modules.size());
    bool no_fragment_shader = true;
    for (auto const s : param.shader_modules) {
        if (s->get_stage() == VK_SHADER_STAGE_FRAGMENT_BIT)
            no_fragment_shader = false;
        stage_info.push_back(VkPipelineShaderStageCreateInfo{
            .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
            .stage = s->get_stage(),
            .module = s->get_handle(),
            .pName = "main",
            .pSpecializationInfo = &special_const_info,
        });
    }
    VkPipelineVertexInputStateCreateInfo input_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
        .flags = 0,
        .vertexBindingDescriptionCount = 0,
        .vertexAttributeDescriptionCount = 0,
    };
    VkPipelineInputAssemblyStateCreateInfo input_assembly_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
        .topology = param.raster_state.topology,
        .primitiveRestartEnable = VK_FALSE,
    };
    VkPipelineTessellationStateCreateInfo tessellation_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
        .patchControlPoints = 0,
    };
    VkPipelineViewportStateCreateInfo viewport_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
        .viewportCount = 1,
        // The spec says:
        // pViewports is a pointer to an array of VkViewport structures,
        // defining the viewport transforms. If the viewport state is dynamic,
        // this member is ignored.
        .pViewports = nullptr,
        .scissorCount = 1,
        .pScissors = nullptr,
    };
    VkPipelineRasterizationStateCreateInfo rasterization_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
        .depthClampEnable = VK_FALSE,
        .rasterizerDiscardEnable = VK_FALSE,
        .polygonMode = param.raster_state.polygon_mode,
        .cullMode = param.raster_state.cull_mode,
        .frontFace = param.raster_state.front_face,
        .depthBiasEnable = param.raster_state.depth_bias_enable,
        .depthBiasConstantFactor =
            param.raster_state.depth_bias_constant_factor,
        .depthBiasClamp = param.raster_state.depth_bias_clamp,
        .depthBiasSlopeFactor = param.raster_state.depth_bias_slope_factor,
        .lineWidth = 1.0f,
    };
    VkPipelineMultisampleStateCreateInfo multisample_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
        .rasterizationSamples = param.raster_state.rasterization_samples,
        .sampleShadingEnable = VK_FALSE,
        .minSampleShading = 0.0f,
        .pSampleMask = nullptr,
        .alphaToCoverageEnable = VK_FALSE,
        .alphaToOneEnable = VK_FALSE,
    };
    VkPipelineDepthStencilStateCreateInfo depth_stencil_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
        .depthTestEnable = VK_TRUE,
        .depthWriteEnable = param.raster_state.depthWrite_enable,
        .depthCompareOp = param.raster_state.depth_compare_op,
        .depthBoundsTestEnable = VK_FALSE,
        .stencilTestEnable = VK_FALSE,
        .minDepthBounds = 0.0f,
        .maxDepthBounds = 0.0f,
    };
    std::array<VkPipelineColorBlendAttachmentState, MAX_ATTACHMENT_COUNT>
        color_blend_attacment{};
    VkPipelineColorBlendStateCreateInfo color_blend_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
        .logicOpEnable = VK_FALSE,
        .logicOp = VK_LOGIC_OP_NO_OP,
        .attachmentCount = 0,
        .pAttachments = color_blend_attacment.data(),
        .blendConstants = {0.0f, 0.0f, 0.0f, 0.0f},
    };
    if (!no_fragment_shader) {
        color_blend_state.attachmentCount =
            param.raster_state.color_target_count;
        for (uint32_t i = 0; i < param.raster_state.color_target_count; ++i) {
            auto& a = color_blend_attacment[i];
            a.blendEnable = param.raster_state.blend_enable;
            a.srcColorBlendFactor = param.raster_state.src_color_blend_factor;
            a.dstColorBlendFactor = param.raster_state.dst_color_blend_factor;
            a.colorBlendOp = param.raster_state.color_blend_op;
            a.srcAlphaBlendFactor = param.raster_state.src_alpha_blend_factor;
            a.dstAlphaBlendFactor = param.raster_state.dst_alpha_blend_factor;
            a.alphaBlendOp = param.raster_state.alpha_blend_op;
            a.colorWriteMask = param.raster_state.color_write_mask;
        }
    }
    std::array<VkDynamicState, 2> dynamic_states = {
        VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamic_state{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
        .dynamicStateCount = (uint32_t) dynamic_states.size(),
        .pDynamicStates = dynamic_states.data(),
    };
    COUST_ASSERT(stage_info.size() > 0, "");
    VkGraphicsPipelineCreateInfo pipeline_info{
        .sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
        .stageCount = (uint32_t) stage_info.size(),
        .pStages = stage_info.data(),
        .pVertexInputState = &input_state,
        .pInputAssemblyState = &input_assembly_state,
        .pTessellationState = &tessellation_state,
        .pViewportState = &viewport_state,
        .pRasterizationState = &rasterization_state,
        .pMultisampleState = &multisample_state,
        .pDepthStencilState = &depth_stencil_state,
        .pColorBlendState = &color_blend_state,
        .pDynamicState = &dynamic_state,
        .layout = layout.get_handle(),
        .renderPass = param.render_pass->get_handle(),
        .subpass = param.subpass,
        .basePipelineHandle = VK_NULL_HANDLE,
    };
    COUST_VK_CHECK(vkCreateGraphicsPipelines(m_dev, cache, 1, &pipeline_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "Can't create vulkan graphics pipeline");
}

VulkanGraphicsPipeline::VulkanGraphicsPipeline(
    VulkanGraphicsPipeline&& other) noexcept
    : m_dev(other.m_dev), m_handle(other.m_handle) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanGraphicsPipeline& VulkanGraphicsPipeline::operator=(
    VulkanGraphicsPipeline&& other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    return *this;
}

VulkanGraphicsPipeline::~VulkanGraphicsPipeline() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

VkDevice VulkanComputePipeline::get_device() const noexcept {
    return m_dev;
}

VkPipeline VulkanComputePipeline::get_handle() const noexcept {
    return m_handle;
}

VulkanComputePipeline::VulkanComputePipeline(VkDevice dev,
    VulkanPipelineLayout const& layout, VkPipelineCache cache,
    Param const& param) noexcept
    : m_dev(dev) {
    auto const specialize_const_info = param.special_const_info.get();
    VkPipelineShaderStageCreateInfo shader_stage_info{
        .sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
        .stage = VK_SHADER_STAGE_COMPUTE_BIT,
        .module = param.shader_module->get_handle(),
        .pName = "main",
        .pSpecializationInfo = &specialize_const_info,
    };
    VkComputePipelineCreateInfo pipeline_info{
        .sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
        .stage = shader_stage_info,
        .layout = layout.get_handle(),
        .basePipelineHandle = VK_NULL_HANDLE,
    };
    COUST_VK_CHECK(vkCreateComputePipelines(m_dev, cache, 1, &pipeline_info,
                       COUST_VULKAN_ALLOC_CALLBACK, &m_handle),
        "");
}

VulkanComputePipeline::VulkanComputePipeline(
    VulkanComputePipeline&& other) noexcept
    : m_dev(other.m_dev), m_handle(other.m_handle) {
    other.m_dev = VK_NULL_HANDLE;
    other.m_handle = VK_NULL_HANDLE;
}

VulkanComputePipeline& VulkanComputePipeline::operator=(
    VulkanComputePipeline&& other) noexcept {
    std::swap(m_dev, other.m_dev);
    std::swap(m_handle, other.m_handle);
    return *this;
}

VulkanComputePipeline::~VulkanComputePipeline() noexcept {
    if (m_handle != VK_NULL_HANDLE) {
        vkDestroyPipeline(m_dev, m_handle, COUST_VULKAN_ALLOC_CALLBACK);
    }
}

static_assert(detail::IsVulkanResource<VulkanPipelineLayout>);
static_assert(detail::IsVulkanResource<VulkanGraphicsPipeline>);
static_assert(detail::IsVulkanResource<VulkanComputePipeline>);

}  // namespace render
}  // namespace coust

namespace std {

std::size_t hash<coust::render::VulkanPipelineLayout::Param>::operator()(
    coust::render::VulkanPipelineLayout::Param const& key) const noexcept {
    size_t h = 0;
    for (auto const shader : key.shader_modules) {
        coust::hash_combine(h, shader->get_byte_code_hash());
    }
    return h;
}

std::size_t hash<VkSpecializationMapEntry>::operator()(
    VkSpecializationMapEntry const& key) const noexcept {
    size_t h = coust::calc_std_hash(key.constantID);
    coust::hash_combine(h, key.offset);
    coust::hash_combine(h, key.size);
    return h;
}

std::size_t hash<coust::render::VulkanGraphicsPipeline::Param>::operator()(
    coust::render::VulkanGraphicsPipeline::Param const& key) const noexcept {
    size_t h = key.special_const_info.get_hash();
    coust::hash_combine(h, key.raster_state.topology);
    coust::hash_combine(h, key.raster_state.polygon_mode);
    coust::hash_combine(h, key.raster_state.cull_mode);
    coust::hash_combine(h, key.raster_state.front_face);
    coust::hash_combine(h, key.raster_state.depth_bias_enable);
    coust::hash_combine(h, key.raster_state.depth_bias_constant_factor);
    coust::hash_combine(h, key.raster_state.depth_bias_clamp);
    coust::hash_combine(h, key.raster_state.depth_bias_slope_factor);
    coust::hash_combine(h, key.raster_state.rasterization_samples);
    coust::hash_combine(h, key.raster_state.depthWrite_enable);
    coust::hash_combine(h, key.raster_state.depth_compare_op);
    coust::hash_combine(h, key.raster_state.color_target_count);
    coust::hash_combine(h, key.raster_state.blend_enable);
    coust::hash_combine(h, key.raster_state.src_color_blend_factor);
    coust::hash_combine(h, key.raster_state.dst_color_blend_factor);
    coust::hash_combine(h, key.raster_state.color_blend_op);
    coust::hash_combine(h, key.raster_state.src_alpha_blend_factor);
    coust::hash_combine(h, key.raster_state.dst_alpha_blend_factor);
    coust::hash_combine(h, key.raster_state.alpha_blend_op);
    coust::hash_combine(h, key.raster_state.color_write_mask);
    for (auto const shader : key.shader_modules) {
        coust::hash_combine(h, shader->get_byte_code_hash());
    }
    coust::hash_combine(h, key.render_pass->get_handle());
    coust::hash_combine(h, key.subpass);
    return h;
}

std::size_t hash<coust::render::VulkanComputePipeline::Param>::operator()(
    coust::render::VulkanComputePipeline::Param const& key) const noexcept {
    size_t h = key.special_const_info.get_hash();
    coust::hash_combine(h, key.shader_module->get_byte_code_hash());
    return h;
}

bool equal_to<coust::render::VulkanPipelineLayout::Param>::operator()(
    coust::render::VulkanPipelineLayout::Param const& left,
    coust::render::VulkanPipelineLayout::Param const& right) const noexcept {
    return std::ranges::equal(left.shader_modules, right.shader_modules,
        [](const coust::render::VulkanShaderModule* l,
            const coust::render::VulkanShaderModule* r) {
            return l->get_handle() == r->get_handle();
        });
}

bool equal_to<coust::render::VulkanGraphicsPipeline::Param>::operator()(
    coust::render::VulkanGraphicsPipeline::Param const& left,
    coust::render::VulkanGraphicsPipeline::Param const& right) const noexcept {
    bool other_bol =
        left.shader_modules.size() == right.shader_modules.size() &&
        left.subpass == right.subpass &&
        left.render_pass->get_handle() == right.render_pass->get_handle();
    if (!other_bol)
        return false;
    if (left.special_const_info != right.special_const_info)
        return false;
    std::span<const uint8_t> raster_l{(const uint8_t*) (&left.raster_state),
        sizeof(coust::render::VulkanGraphicsPipeline::RasterState)};
    std::span<const uint8_t> raster_r{(const uint8_t*) (&right.raster_state),
        sizeof(coust::render::VulkanGraphicsPipeline::RasterState)};
    if (!std::ranges::equal(raster_l, raster_r))
        return false;
    return std::ranges::equal(left.shader_modules, right.shader_modules,
        [](const coust::render::VulkanShaderModule* const l,
            const coust::render::VulkanShaderModule* const r) {
            return l->get_handle() == r->get_handle();
        });
}

bool equal_to<coust::render::VulkanComputePipeline::Param>::operator()(
    coust::render::VulkanComputePipeline::Param const& left,
    coust::render::VulkanComputePipeline::Param const& right) const noexcept {
    return left.special_const_info == right.special_const_info &&
           left.shader_module->get_handle() ==
               right.shader_module->get_handle();
}

}  // namespace std
