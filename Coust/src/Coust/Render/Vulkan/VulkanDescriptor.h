#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

namespace Coust::Render::VK
{
    struct ShaderResource;
    class ShaderModule;

    class DescriptorSetLayout;
    class DescriptorSet;
    class DescriptorSetAllocator;

    class DescriptorSetLayout : public Resource<VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>,
                                public Hashable
    {
    public:
        using Base = Resource<VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>;
    
    public:
        struct ConstructParam
        {
            const Context&                                      ctx;
            uint32_t                                            set;
            const std::vector<std::shared_ptr<ShaderModule>>&   shaderModules;
            const std::vector<ShaderResource>&                  shaderResources;
            const char*                                         scopeName = nullptr;
            const char*                                         dedicatedName = nullptr;
        };
        /**
         * @brief Construct a descriptor set layout with default debug name
         * 
         * @param ctx 
         * @param set                   Descriptor set number. Usually a unique descriptor set layout is corresponding to a unique set number, so we store it in the layout for convenience.
         * @param shaderModules         The correspondent shader modules. Passed for hashing.
         * @param shaderResources       Shader resources from SPIR-V reflection contains binding information for construction.
         * @param scopeName
         * @param dedicatedName
         */
        DescriptorSetLayout(ConstructParam param);

        ~DescriptorSetLayout();
        
        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
        
        DescriptorSetLayout() = delete;
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) = delete;
        
        uint32_t GetSetIndex() const { return m_Set; }
        
        const std::vector<VkDescriptorSetLayoutBinding>& GetBindings() const { return m_CompactBindings; }
        
        std::optional<VkDescriptorSetLayoutBinding> GetBinding(uint32_t bindingIdx) const;

        std::optional<VkDescriptorSetLayoutBinding> GetBinding(const std::string& name) const;
        
    private:
        /**
         * @brief Actual construction happens here
         * 
         * @param ctx 
         * @param shaderModules 
         * @param shaderResources 
         * @return false if construction failed
         */
        bool Construct(const Context& ctx, const std::vector<std::shared_ptr<ShaderModule>>& shaderModules, const std::vector<ShaderResource>& shaderResources);
        
    private:
        uint32_t m_Set;
        
        /**
         * @brief Containing all bindings for SEARCHING, whose index is its binding index
         */
        std::vector<VkDescriptorSetLayoutBinding> m_Bindings{};
        /**
         * @brief `m_Bindings` gotten rid of redundant (or empty) bindings
         */
        std::vector<VkDescriptorSetLayoutBinding> m_CompactBindings{};

        /**
         * @brief shader resource name -> binding index. So given a resource name, its binding information would be `m_Bindings[m_ResNameToBindingIdx[name]]`
         */
        std::unordered_map<std::string, uint32_t> m_ResNameToBindingIdx{};
    };
    
    class DescriptorSet : public Resource<VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET>
    {
    public:
        using Base = Resource<VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET>;
        
    public:
        // DescriptorSet(const Context& ctx, const DescriptorSetLayout& layout, const DescriptorSetAllocator& allocator);
        
    };
    
    /**
     * @brief A free list of descriptor pool, perform on-demand allocation of descriptor set. 
     */
    class DescriptorSetAllocator : public Hashable
    {
    public:
        static constexpr uint32_t MAX_SET_PER_POOL = 16;

    public:
        struct ConstructParam
        {
            const Context& ctx;
            const DescriptorSetLayout& layout;
            uint32_t maxSets = MAX_SET_PER_POOL;
        };
        /**
         * @brief Construct a descriptor set allocator
         * 
         * @param ctx 
         * @param layout 
         * @param maxSets 
         */
        DescriptorSetAllocator(ConstructParam param);
        
        ~DescriptorSetAllocator();
        
        DescriptorSetAllocator() = delete;
        DescriptorSetAllocator(DescriptorSetAllocator&&) = delete;
        DescriptorSetAllocator(const DescriptorSetAllocator&) = delete;
        DescriptorSetAllocator& operator=(DescriptorSetAllocator&&) = delete;
        DescriptorSetAllocator& operator=(const DescriptorSetAllocator&) = delete;
        
        const DescriptorSetLayout& GetLayout() const { return *m_Layout; }
        
        /**
         * @brief Allocate a descriptor set, and notice that we don't keep track of the allocated descirptor sets. 
         * @return VkDescriptorSet 
         */
        VkDescriptorSet Allocate();
        
        /**
         * @brief Reset the whole pools of descriptor pool
         */
        void Reset();
        
    private:
        /**
         * @brief Find if there's any available pool *AFTER* `m_CurrentFactoryIdx`, if there is, then only update `m_CurrentFactoryIdx`
                  if there isn't, update `m_CurrentFactoryIdx` and allocate a new descriptor pool
         * @return false if the func `vkCreateDescriptorPool` failed
         */
        bool RequireFactory();

    private:
        struct Factory 
        {
            Resource<VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL> Pool;
            uint32_t CurrentProductCount = 0;
            
            Factory(VkDevice device, VkDescriptorPool handle)
                : Pool(device, handle)
            {
                Pool.SetDefaultDebugName("Coust::VK::DescriptorAllocator", nullptr);
            }
            Factory() = delete;
        };
        
    private:
        
        uint32_t m_MaxSetsPerPool;
        
        uint32_t m_CurrentFactoryIdx = 0;
        
        VkDevice m_Device;
        
        const DescriptorSetLayout* m_Layout;

        std::vector<VkDescriptorPoolSize> m_PoolSizes;
        
        std::vector<Factory> m_Factories;

        std::vector<VkDescriptorSet> m_Products;
    };
}