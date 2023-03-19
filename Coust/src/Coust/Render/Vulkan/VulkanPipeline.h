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
            std::vector<VkVertexInputBindingDescription>        vertexBindingDescriptions;
            std::vector<VkVertexInputAttributeDescription>      vertexAttributeDescriptions;
            // We don't use primitive restart since most of the time we will use the triangle list, as the spec says:
            // primitiveRestartEnable controls whether a special vertex index value is treated as restarting the
            // assembly of primitives. This enable only applies to indexed draws (vkCmdDrawIndexed, and
            // vkCmdDrawIndexedIndirect), and the special index value is either 0xFFFFFFFF when the
            // indexType parameter of vkCmdBindIndexBuffer is equal to VK_INDEX_TYPE_UINT32, or 0xFFFF when
            // indexType is equal to VK_INDEX_TYPE_UINT16. Primitive restart is not allowed for list topologies.
            // const VkPipelineInputAssemblyStateCreateInfo*    pInputAssemblyState;
            VkPrimitiveTopology                                 topology;
            // We don't support tesselletion now
            // Viewport & scissor are dynamic states, their setting is postponed until actual drawing
            // We don't use depth clamp & rasterizer discard, and lineWidth is always 1.0f
            VkPolygonMode                                       polygonMode;
            VkCullModeFlags                                     cullMode;
            VkFrontFace                                         frontFace;
            VkBool32                                            depthBiasEnable;
            float                                               depthBiasConstantFactor;
            float                                               depthBiasClamp;
            float                                               depthBiasSlopeFactor;
            // We don't use sample shading or multisample coverage
            VkSampleCountFlagBits                               rasterizationSamples;
            // We don't use depth bounds test and stencil test, and the depth test is always enabled
            VkBool32                                            depthWriteEnable;
            // https://github.com/KhronosGroup/Vulkan-Samples/pull/25
            // TODO: Use Reversed depth-buffer get more even distribution of precision
            VkCompareOp                                         depthCompareOp;
            // We don't use logical operation and constant blend value
            // Also, we use consistent color blend across all the render targets
            VkBool32                                            blendEnable;
            VkBlendFactor                                       srcColorBlendFactor;
            VkBlendFactor                                       dstColorBlendFactor;
            VkBlendOp                                           colorBlendOp;
            VkBlendFactor                                       srcAlphaBlendFactor;
            VkBlendFactor                                       dstAlphaBlendFactor;
            VkBlendOp                                           alphaBlendOp;
            VkColorComponentFlags                               colorWriteMask;
            // We use fixed dynamic state which contains viewport and scissor
            PipelineLayout&                                     layout;
            RenderPass&                                         renderPass;
            uint32_t                                            subpassIdx;
            const char*                                         scopeName = nullptr;
            const char*                                         dedicatedName = nullptr;

            size_t GetHash() const;
        };
        GraphicsPipeline(const ConstructParam& param);

        GraphicsPipeline(GraphicsPipeline&& other);

        ~GraphicsPipeline();

    private:
    };
}
