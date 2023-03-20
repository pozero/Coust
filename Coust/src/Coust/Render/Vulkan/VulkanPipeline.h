#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <vector>
#include <unordered_set>
#include <compare>

namespace Coust::Render::VK
{
    class RenderPass;
    class ShaderModule;
    struct ShaderResource;
    class DescriptorSetLayout;

    class PipelineState;
    class PipelineLayout;
    
    class PipelineLayout : public Resource<VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT>,
                           public Hashable
    {
    public:
        using Base = Resource<VkPipelineLayout, VK_OBJECT_TYPE_PIPELINE_LAYOUT>;

        PipelineLayout() = delete;
        PipelineLayout(const PipelineLayout&) = delete;
        PipelineLayout& operator=(PipelineLayout&&) = delete;
        PipelineLayout& operator=(const PipelineLayout&) = delete;

    public:
        struct ConstructParam 
        {
            const Context&                      ctx;
            std::vector<ShaderModule*>          shaderModules;
            const char*                         scopeName = nullptr;
            const char*                         dedicatedName = nullptr;

            size_t GetHash() const;
        };
        PipelineLayout(const ConstructParam& param);

        PipelineLayout(PipelineLayout&& other);

        ~PipelineLayout();

        const std::vector<ShaderModule*>& GetShaderModules() const;

        const std::vector<DescriptorSetLayout>& GetDescriptorSetLayouts() const;

        const std::vector<ShaderResource>& GetShaderResources() const;

    private:
        std::vector<ShaderModule*> m_ShaderModules;

        // A group of shader module corresponds to a  group of descriptor set layouts, it doesn't make much sense to cache these layout because of this multi to multi correlation.
        // So here we let the pipeline layout MANAGES these descriptor set layouts.
        std::vector<DescriptorSetLayout> m_DescriptorLayouts;
        
        std::vector<ShaderResource> m_ShaderResources;
        // set -> mask of indice in `out_AllShaderResources` of all resources in this set
        std::unordered_map<uint32_t, uint64_t> m_SetToResourceIdxLookup;
    };

    // This class conveys per-pipeline specialization constant information, because there won't be much specialization constants durng creation. There's no need to store them per shader.
    // So all the shader modules inside one pipeline will share the same constant id.
    class SpecializationConstantInfo
    {
    public:
        template <typename T>
        bool AddConstant(uint32_t id, const T& data)
        {
            if (auto iter = std::find_if(m_Entry.begin(), m_Entry.end(), 
                [id](decltype(m_Entry[0]) i) -> bool
                {
                    return i.constantID == id;
                }); iter != m_Entry.end())
            {
                COUST_CORE_WARN("The data for specialization constant of id {} already exists", id);
                return false;
            }

            m_Entry.push_back(
                VkSpecializationMapEntry
                {
                    .constantID = id,
                    .offset = (uint32_t) m_Data.size(),
                    .size = sizeof(T),
                });
            std::copy(&data, &data + sizeof(data), std::back_inserter(m_Data));

            return true;
        }

        template <typename T>
        bool ChangeConstant(uint32_t id, const T& data)
        {
            if (auto iter = std::find_if(m_Entry.begin(), m_Entry.end(), 
                [id](decltype(m_Entry[0]) i) -> bool
                {
                    return i.constantID == id;
                }); iter == m_Entry.end())
                return AddConstant(id, data);
            else
            {
                if (iter->size != sizeof(T))
                {
                    COUST_CORE_ERROR("The size of previous data for specialization constant doesn't match the size of new data");
                    return false;
                }

                T* entry = (T*) (m_Data.data() + iter->offset);
                *entry = data;
                return true;
            }
        }

        VkSpecializationInfo Get() const;

        size_t GetHash() const;

    private:
        std::vector<VkSpecializationMapEntry> m_Entry;
        std::vector<uint8_t> m_Data;
    };

    // post processing
    // https://community.arm.com/arm-community-blogs/b/graphics-gaming-and-vr-blog/posts/using-compute-post-processing-in-vulkan-on-mali

    class GraphicsPipeline : public Resource<VkPipeline, VK_OBJECT_TYPE_PIPELINE>,
                             public Hashable
    {
    public:
        using Base = Resource<VkPipeline, VK_OBJECT_TYPE_PIPELINE>;
        GraphicsPipeline() = delete;
        GraphicsPipeline(const GraphicsPipeline&) = delete;
        GraphicsPipeline& operator=(GraphicsPipeline&&) = delete;
        GraphicsPipeline& operator=(const GraphicsPipeline&) = delete;

    public:
        struct ConstructParam 
        {
            const Context&                                      ctx;
            const SpecializationConstantInfo*                   specializationConstantInfo = nullptr;
            // We don't use primitive restart since most of the time we will use the triangle list, as the spec says:
            // primitiveRestartEnable controls whether a special vertex index value is treated as restarting the
            // assembly of primitives. This enable only applies to indexed draws (vkCmdDrawIndexed, and
            // vkCmdDrawIndexedIndirect), and the special index value is either 0xFFFFFFFF when the
            // indexType parameter of vkCmdBindIndexBuffer is equal to VK_INDEX_TYPE_UINT32, or 0xFFFF when
            // indexType is equal to VK_INDEX_TYPE_UINT16. Primitive restart is not allowed for list topologies.
            // const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
            VkPrimitiveTopology                                 topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
            // We don't support tesselletion now
            // Viewport & scissor are dynamic states, their setting is postponed until actual drawing
            // We don't use depth clamp & rasterizer discard, and lineWidth is always 1.0f
            VkPolygonMode                                       polygonMode = VK_POLYGON_MODE_FILL;
            VkCullModeFlags                                     cullMode = VK_CULL_MODE_BACK_BIT;
            VkFrontFace                                         frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
            VkBool32                                            depthBiasEnable = VK_FALSE;
            float                                               depthBiasConstantFactor = 0.0f;
            float                                               depthBiasClamp = 0.0f;
            float                                               depthBiasSlopeFactor = 0.0f;
            // We don't use sample shading or multisample coverage
            VkSampleCountFlagBits                               rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
            // We don't use depth bounds test and stencil test, and the depth test is always enabled
            VkBool32                                            depthWriteEnable = VK_TRUE;
            // https://github.com/KhronosGroup/Vulkan-Samples/pull/25
            // TODO: Use Reversed depth-buffer get more even distribution of precision
            VkCompareOp                                         depthCompareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
            // We don't use logical operation and constant blend value
            // Also, we use consistent color blend across all the render targets
            uint32_t                                            colorTargetCount = 1;
            VkBool32                                            blendEnable = VK_FALSE;
            VkBlendFactor                                       srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
            VkBlendFactor                                       dstColorBlendFactor = VK_BLEND_FACTOR_ZERO;
            VkBlendOp                                           colorBlendOp = VK_BLEND_OP_ADD;
            VkBlendFactor                                       srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
            VkBlendFactor                                       dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
            VkBlendOp                                           alphaBlendOp = VK_BLEND_OP_ADD;
            VkColorComponentFlags                               colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
            // We use fixed dynamic state which contains viewport and scissor
            PipelineLayout&                                     layout;
            RenderPass&                                         renderPass;
            uint32_t                                            subpassIdx;
            VkPipelineCache                                     cache = VK_NULL_HANDLE;
            const char*                                         scopeName = nullptr;
            const char*                                         dedicatedName = nullptr;

            size_t GetHash() const;
        };
        GraphicsPipeline(const ConstructParam& param);

        GraphicsPipeline(GraphicsPipeline&& other);

        ~GraphicsPipeline();

        const PipelineLayout& GetLayout() const;

        const RenderPass& GetRenderPass() const;

        uint32_t GetSubpassIndex() const;

    private:
        const PipelineLayout& m_Layout;

        const RenderPass& m_RenderPass;

        const uint32_t m_SubpassIdx;
    };
}
