#include "pch.h"

#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanShader.h"

#include "Coust/Utils/Hash.h"

namespace Coust::Render::VK
{
    inline VkDescriptorType GetDescriptorType(ShaderResourceType type, ShaderResourceUpdateMode mode)
    {
        switch (type) 
        {
            case ShaderResourceType::InputAttachment:       return VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
            case ShaderResourceType::Image:                 return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
            case ShaderResourceType::Sampler:               return VK_DESCRIPTOR_TYPE_SAMPLER;
            case ShaderResourceType::ImageAndSampler:       return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            case ShaderResourceType::ImageStorage:          return VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            case ShaderResourceType::UniformBuffer: 
                return mode == ShaderResourceUpdateMode::Dynamic ? 
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC : 
                    VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            case ShaderResourceType::StorageBuffer: 
                return mode == ShaderResourceUpdateMode::Dynamic ? 
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC : 
                    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            default:
                COUST_CORE_ERROR("Can't find suitable `VkDescriptorType` for {}", ToString(type));
                return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout::ConstructParam0 param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), m_Set(param.set)
    {
        Construct(param.ctx, param.shaderModules, param.shaderResources);
        if (m_Handle != VK_NULL_HANDLE)
        {
            SetDefaultDebugName(param.scopeName, nullptr);
        }
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout::ConstructParam1 param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), m_Set(param.set)
    {
        Construct(param.ctx, param.shaderModules, param.shaderResources);
        if (m_Handle != VK_NULL_HANDLE)
        {
            SetDedicatedDebugName(param.name);
        }
    }

    DescriptorSetLayout::~DescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(m_Device, m_Handle, nullptr);
    }
        
    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other)
        : Base(std::forward<Base>(other)), 
        m_Set(other.m_Set), 
        m_Hash(other.m_Hash), 
        m_Bindings(other.m_Bindings), 
        m_CompactBindings(other.m_CompactBindings), 
        m_ResNameToBindingIdx(other.m_ResNameToBindingIdx)
    {
    }

    void DescriptorSetLayout::Construct(const Context& ctx, 
                                        const std::vector<std::unique_ptr<ShaderModule>>& shaderModules, 
                                        const std::vector<ShaderResource>& shaderResources)
    {
        m_Hash = 0;
        for (const auto& s : shaderModules)
        {
            Hash::Combine(m_Hash, *s);
        }

        constexpr size_t vectorGrowGranularity = 5;
        m_Bindings.resize(vectorGrowGranularity);
        for (const auto& res : shaderResources)
        {
            // skip resources without binding poinshaderHasht
            if (res.Type == ShaderResourceType::Input ||
                res.Type == ShaderResourceType::Output ||
                res.Type == ShaderResourceType::PushConstant ||
                res.Type == ShaderResourceType::SpecializationConstant)
                continue;
            
            VkDescriptorType descriptorType = GetDescriptorType(res.Type, res.UpdateMode);
            VkDescriptorSetLayoutBinding binding
            {
                .binding            = res.Binding,
                .descriptorType     = descriptorType,
                .descriptorCount    = res.ArraySize,
                .stageFlags         = res.Stage,
            };
            
            if (m_Bindings.size() - 1 < res.Binding)
                m_Bindings.resize(m_Bindings.size() + vectorGrowGranularity);
            // We might accidently specify different resources with the same set and binding, log it
            else if (m_Bindings[res.Binding].stageFlags != 0)
            {
                COUST_CORE_ERROR("There are different shader resources with the same binding while constructing {}. The binding is: Name {} -> Set {}, Binding {}", m_DebugName, res.Name, m_Set, res.Binding);
                return;
            }

            m_Bindings[res.Binding] = binding;
            m_ResNameToBindingIdx.emplace(res.Name, res.Binding);
        }
        
        for (const auto& b : m_Bindings)
        {
            if (b.stageFlags != 0)
                m_CompactBindings.push_back(b);
        }
        
        VkDescriptorSetLayoutCreateInfo ci 
        {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags              = 0,
            .bindingCount       = (uint32_t) m_CompactBindings.size(),
            .pBindings          = m_CompactBindings.data(),
        };
        bool succeeded = false;
        VK_REPORT(vkCreateDescriptorSetLayout(ctx.Device, &ci, nullptr, &m_Handle), succeeded);

        if (!succeeded)
            m_Handle = VK_NULL_HANDLE;
    }

    VkDescriptorSetLayoutBinding DescriptorSetLayout::GetBinding(uint32_t bindingIdx) const
    {
        if (bindingIdx >= m_Bindings.size())
        {
            COUST_CORE_WARN("{}: bindingIdx out of bounds", m_DebugName);
            return VkDescriptorSetLayoutBinding{};
        }
        else  
        {
            auto res = m_Bindings[bindingIdx];
            if (res.stageFlags == 0)
                COUST_CORE_WARN("{}: Requesting a non-existent descriptor set binding. The binding is: Set {}, Binding {}", m_DebugName, m_Set, bindingIdx);
            return res;
        }
    }
    
    VkDescriptorSetLayoutBinding DescriptorSetLayout::GetBinding(const std::string& name) const
    {
        auto iter = m_ResNameToBindingIdx.find(name);
        if (iter != m_ResNameToBindingIdx.end())
            return m_Bindings[m_ResNameToBindingIdx.at(name)];
        else
        {
            COUST_CORE_WARN("{}: Requesting a non-existent resource. The requested resource name is {}", m_DebugName, name);
            return VkDescriptorSetLayoutBinding{};
        }
    }
}
