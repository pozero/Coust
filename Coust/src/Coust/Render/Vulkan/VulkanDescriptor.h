#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

namespace Coust::Render::VK
{
    struct ShaderResource;
    class ShaderModule;
    class DescriptorSetLayout;

    class DescriptorSetLayout : public Resource<VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>
    {
    public:
        using Base = Resource<VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>;
    
    public:
        struct ConstructParam0
        {
            const Context&                                      ctx;
            uint32_t                                            set;
            const std::vector<std::unique_ptr<ShaderModule>>&   shaderModules;
            const std::vector<ShaderResource>&                  shaderResources;
            const char*                                         scopeName;
        };
        /**
         * @brief Construct a descriptor set layout with default debug name
         * 
         * @param ctx 
         * @param set                   Descriptor set number. Usually a unique descriptor set layout is corresponding to a unique set number, so we store it in the layout for convenience.
         * @param shaderModules         The correspondent shader modules. Passed for hashing.
         * @param shaderResources       Shader resources from SPIR-V reflection contains binding information for construction.
         * @param scopeName
         */
        DescriptorSetLayout(ConstructParam0 param);

        struct ConstructParam1
        {
            const Context&                                      ctx;
            uint32_t                                            set;
            const std::vector<std::unique_ptr<ShaderModule>>&   shaderModules;
            const std::vector<ShaderResource>&                  shaderResources;
            const char*                                         name;
        };
        /**
         * @brief Construct a descriptor set layout with dedicated debug name
         * 
         * @param ctx 
         * @param set                   Descriptor set number. Usually a unique descriptor set layout is corresponding to a unique set number, so we store it in the layout for convenience.
         * @param shaderModules         The correspondent shader modules. Passed for hashing.
         * @param shaderResources       Shader resources from SPIR-V reflection contains binding information for construction.
         * @param name
         */
        DescriptorSetLayout(ConstructParam1 param);
        
        ~DescriptorSetLayout();
        
        DescriptorSetLayout(DescriptorSetLayout&& other);
        
        DescriptorSetLayout() = delete;
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) = delete;
        
        size_t GetHash() const { return m_Hash; }
        
        uint32_t GetSetIndex() const { return m_Set; }
        
        const std::vector<VkDescriptorSetLayoutBinding>& GetBindings() const { return m_CompactBindings; }
        
        VkDescriptorSetLayoutBinding GetBinding(uint32_t bindingIdx) const;
        
        VkDescriptorSetLayoutBinding GetBinding(const std::string& name) const;
        
    private:
        /**
         * @brief Actual construction happens here
         * 
         * @param ctx 
         * @param shaderModules 
         * @param shaderResources 
         */
        void Construct(const Context& ctx, const std::vector<std::unique_ptr<ShaderModule>>& shaderModules, const std::vector<ShaderResource>& shaderResources);
        
    private:
        uint32_t m_Set;
        size_t m_Hash;
        
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
}