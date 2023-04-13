#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"

#include "Coust/Utils/Hash.h"

#include <map>
#include <vector>
#include <unordered_set>
#include <unordered_map>

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
            const Context*                              ctx = nullptr;              // Here we use poiter because we need to be able to clear this class in pipeline cache, which is prohibited by reference.
            std::vector<ShaderModule*>                  shaderModules;              // Passed for hashing
            std::vector<ShaderResource>                 shaderResources;
            std::unordered_map<uint32_t, uint64_t>      setToResourceIdxLookup;
            const char*                                 scopeName = nullptr;
            const char*                                 dedicatedName = nullptr;

            size_t GetHash() const noexcept;
        };
        explicit PipelineLayout(const ConstructParam& param) noexcept;

        PipelineLayout(PipelineLayout&& other) noexcept;

        ~PipelineLayout() noexcept;

        const std::vector<ShaderModule*>& GetShaderModules() const noexcept;

        const std::vector<DescriptorSetLayout>& GetDescriptorSetLayouts() const noexcept;

        const std::vector<ShaderResource>& GetShaderResources() const noexcept;

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
        SpecializationConstantInfo() = default;
        ~SpecializationConstantInfo() = default;
        SpecializationConstantInfo(SpecializationConstantInfo&&) = default;

        SpecializationConstantInfo(const SpecializationConstantInfo&) = delete;
        SpecializationConstantInfo& operator=(SpecializationConstantInfo&&) = delete;
        SpecializationConstantInfo& operator=(const SpecializationConstantInfo&) = delete;

    public:
        template <typename T>
        bool AddConstant(uint32_t id, const T& data) noexcept
        {
            if (auto iter = std::find_if(m_Entry.begin(), m_Entry.end(), 
                [id](decltype(*m_Entry.cbegin()) i) -> bool
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
        bool ChangeConstant(uint32_t id, const T& data) noexcept
        {
            if (auto iter = std::find_if(m_Entry.begin(), m_Entry.end(), 
                [id](decltype(*m_Entry.cbegin()) i) -> bool
                {
                    return i.constantID == id;
                }); iter == m_Entry.end())
                return AddConstant(id, data);
            else
            {
                if (iter->size != sizeof(T))
                {
                    COUST_CORE_ERROR("The size of previous data for specialization constant doesn't match the size of the new data");
                    return false;
                }

                T* entry = (T*) (m_Data.data() + iter->offset);
                *entry = data;
                return true;
            }
        }

        void Reset() noexcept;

        VkSpecializationInfo Get() const noexcept;

        size_t GetHash() const noexcept;

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
        struct RasterState
        {
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
        };

        struct ConstructParam 
        {   
            const Context*                                      ctx = nullptr;                          // Here we use poiter because we need to be able to clear this class in pipeline cache, which is prohibited by reference.
            const SpecializationConstantInfo*                   specializationConstantInfo = nullptr;
            uint32_t                                            perInstanceInputMask = 0u;
            RasterState                                         rasterState;
            const PipelineLayout*                               layout = nullptr;
            const RenderPass*                                   renderPass = nullptr;
            uint32_t                                            subpassIdx;
            VkPipelineCache                                     cache = VK_NULL_HANDLE;
            const char*                                         scopeName = nullptr;
            const char*                                         dedicatedName = nullptr;

            size_t GetHash() const noexcept;
        };
        explicit GraphicsPipeline(const ConstructParam& param) noexcept;

        GraphicsPipeline(GraphicsPipeline&& other) noexcept;

        ~GraphicsPipeline() noexcept;

        const PipelineLayout& GetLayout() const noexcept;

        const RenderPass& GetRenderPass() const noexcept;

        uint32_t GetSubpassIndex() const noexcept;

    private:
        const PipelineLayout& m_Layout;

        const RenderPass& m_RenderPass;

        const uint32_t m_SubpassIdx;
    };

    // This cache class is reponsible for tracking the state of rendering, including shaders, pipelines, descriptors, shader resources, etc
    class GraphicsPipelineCache
    {
    public:
        GraphicsPipelineCache() = delete;
        GraphicsPipelineCache(GraphicsPipelineCache&&) = delete;
        GraphicsPipelineCache(const GraphicsPipelineCache&) = delete;
        GraphicsPipelineCache& operator=(GraphicsPipelineCache&&) = delete;
        GraphicsPipelineCache& operator=(const GraphicsPipelineCache&) = delete;

    public:
        explicit GraphicsPipelineCache(const Context& ctx) noexcept;

        // Clean every thing that's been cached
        void Reset() noexcept;

        // This function will be called when the driver switches to a new command buffer, which means the old command buffer is submitted.
        // We know that command buffer records all these bindings, so once it's submitted, all the old render states bound previously are gone.
        // We would do some housekeeping here, clearing out all the old render states.
        void GC(const CommandBuffer& buf) noexcept;

        // Internal clearing methods. They just set the corresponding requirements to default state.

        void UnBindDescriptorSet() noexcept;

        void UnBindPipeline() noexcept;

        // Internal binding methods. They don't issue any vulkan call.
        // Since we use reflection to manage our resource binding, some of the following functions require strict calling order.
        // Basically, all function related to shader resoure binding should be called AFTER the binding of pipeline layout (which contains descriptor set layout) finished, 
        // and actual vulkan binding should always happens at the very end.
        // Ideally, the call order should be the same as the order they present below.

        // Get the specialization constant info and modify it.
        // Specialization constant info will get cleared when a new command buffer is activated.
        SpecializationConstantInfo& BindSpecializationConstant() noexcept;

        bool BindShader(const ShaderModule::ConstructParm& source) noexcept;

        // Tell the cache that's all the shader we need. ONLY after this function can we start to bind shader resources.
        // This function will collect all the shader resources for pipeline layout construction,
        // and clear the update mode of all the shader resources inside the current bound shader modules to `Static`.
        void BindShaderFinished() noexcept;

        // Modify the update mode of sepcific buffer
        void SetAsDynamic(std::string_view name) noexcept;

        // Tell the cache that's all the shader resource (with correct update mode) we need, and build or get the pipeline layout and corresponding descriptor set allocator.
        bool BindPipelineLayout() noexcept;

        void BindRasterState(const GraphicsPipeline::RasterState& state) noexcept;

        void BindRenderPass(const RenderPass* renderPass, uint32_t subpassIdx) noexcept;

        // The following functions are responsible for configuring the construction parameter for descriptor sets

        void BindBuffer(std::string_view name, const Buffer& buffer, uint64_t offset, uint64_t size, uint32_t arrayIdx) noexcept;

        void BindImage(std::string_view name, VkSampler sampler, const Image& image, uint32_t arrayIdx) noexcept;

        void BindInputAttachment(std::string_view name, const Image& attachment) noexcept;

        void SetInputRatePerInstance(uint32_t location) noexcept;

        // Actual binding
        
        bool BindDescriptorSet(VkCommandBuffer cmdBuf) noexcept;

        bool BindPipeline(VkCommandBuffer cmdBuf) noexcept;

    private:
        void CreateDescriptorAllocator() noexcept;

        void FillDescriptorSetRequirements() noexcept;

    private:
        // We won't recycle shader modules here, since they won't get much change during rendering and compile a shader from source takes huge amount of time.
        // Also note that we can NOT change the update mode of uniform buffer or storage buffer inside the shader module. 
        // Instead we give caller a chance to change the update mode BEFORE we actually get pipeline layout, and hash the update mode information in the pipeline layout class.
        std::vector<ShaderModule> m_CachedShaderModules;

        // construction hash -> { pipeline layout, last accessed time }
        std::unordered_map<size_t, std::pair<PipelineLayout, uint32_t>> m_CachedPipelineLayouts;
        // pipeline layout -> corresponding descriptor set allocators:
        //      set index -> descriptor set allocator
        std::unordered_map<const PipelineLayout*, std::vector<DescriptorSetAllocator>> m_DescriptorSetAllocators;

        // The descriptor sets in the `m_CachedDescriptorSets` are filled with write info, which means they are bound to specific buffers & images,
        // whilst the descriptor sets stored in the free list do not carry any write info.
        // construction hash -> { used decriptor sets, last accessed time }
        std::unordered_map<size_t, std::pair<DescriptorSet, uint32_t>> m_CachedDescriptorSets;

        // construction hash -> { graphics pipline, last accessed time }
        std::unordered_map<size_t, std::pair<GraphicsPipeline, uint32_t>> m_CachedGraphicsPipelines;

        // Following are currently bound resources, which are basically index to the cache above.

        SpecializationConstantInfo m_SpecializationConstantCurrent{};
        std::unordered_set<uint32_t> m_CurrentBoundShaderModules{};
        PipelineLayout* m_PipelineLayoutCurrent = nullptr;
        GraphicsPipeline* m_GraphicsPipelineCurrent = nullptr;
        std::vector<DescriptorSet*> m_DescriptorSetsCurrent{};
        // set index -> dynamic offset
        std::unordered_map<uint32_t, uint32_t> m_DynamicOffsets;

        // Following are requirements for current drawing iteration, at stage where actual binding happens, the code will check if the current binding can fulfill these
        // requirements, if so, then do nothing.

        PipelineLayout::ConstructParam m_PipelineLayoutRequirement;
        GraphicsPipeline::ConstructParam m_GraphicsPipelineRequirement;
        std::vector<DescriptorSet::ConstructParam> m_DescriptorSetRequirement;

        const Context& m_Ctx;

        EvictTimer m_Timer;

        CacheHitCounter m_PipelineLayoutHitCounter;
        CacheHitCounter m_DescriptorSetHitCounter;
        CacheHitCounter m_GraphicsPipelineHitCounter;

        // TODO: load & save pipeline cache
        VkPipelineCache m_Cache = VK_NULL_HANDLE;
    };
}
