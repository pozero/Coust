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

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout::ConstructParam param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), Hashable(0), m_Set(param.set)
    {
        if (Construct(param.ctx, param.shaderModules, param.shaderResources))
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, nullptr);
            else
                COUST_CORE_WARN("Descriptor set layout created without a debug name");
        }
        else  
            m_Handle = VK_NULL_HANDLE;
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
        : Base(std::forward<Base>(other)), Hashable(std::forward<Hashable>(other)),
          m_Set(other.m_Set), 
          m_Bindings(other.m_Bindings), 
          m_CompactBindings(other.m_CompactBindings), 
          m_ResNameToBindingIdx(other.m_ResNameToBindingIdx)
    {
    }


    DescriptorSetLayout::~DescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(m_Device, m_Handle, nullptr);
    }

    bool DescriptorSetLayout::Construct(const Context& ctx, 
                                        const std::vector<std::shared_ptr<ShaderModule>>& shaderModules, 
                                        const std::vector<ShaderResource>& shaderResources)
    {
        m_Hash = 0;
        for (const auto& s : shaderModules)
        {
            Hash::Combine(m_Hash, *s);
        }

        constexpr size_t vectorGrowthGranularity = 5;
        m_Bindings.resize(vectorGrowthGranularity);
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
                // .pImmutableSamplers = nullptr,
            };
            
            if (m_Bindings.size() - 1 < res.Binding)
                m_Bindings.resize(m_Bindings.size() + vectorGrowthGranularity);
            // We might accidently specify different resources with the same set and binding, log it
            else if (m_Bindings[res.Binding].stageFlags != 0)
            {
                COUST_CORE_ERROR("There are different shader resources with the same binding while constructing {}. The binding is: Name {} -> Set {}, Binding {}", m_DebugName, res.Name, m_Set, res.Binding);
                return false;
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
        VK_CHECK(vkCreateDescriptorSetLayout(ctx.Device, &ci, nullptr, &m_Handle));
        return true;
    }

    std::optional<VkDescriptorSetLayoutBinding> DescriptorSetLayout::GetBinding(uint32_t bindingIdx) const
    {
        if (bindingIdx >= m_Bindings.size())
        {
            COUST_CORE_WARN("{}: bindingIdx out of bounds", m_DebugName);
            return {};
        }
        else  
        {
            auto res = m_Bindings[bindingIdx];
            if (res.stageFlags == 0)
            {
                COUST_CORE_WARN("{}: Requesting a non-existent descriptor set binding. The binding is: Set {}, Binding {}", m_DebugName, m_Set, bindingIdx);
                return {};
            }
            return res;
        }
    }
    
    std::optional<VkDescriptorSetLayoutBinding> DescriptorSetLayout::GetBinding(const std::string& name) const
    {
        auto iter = m_ResNameToBindingIdx.find(name);
        if (iter != m_ResNameToBindingIdx.end())
            return m_Bindings[m_ResNameToBindingIdx.at(name)];
        else
        {
            COUST_CORE_WARN("{}: Requesting a non-existent resource. The requested resource name is {}", m_DebugName, name);
            return {};
        }
    }
    
    DescriptorSetAllocator::DescriptorSetAllocator(DescriptorSetAllocator::ConstructParam param)
        // We use the same hash value as the descriptor set layout
        : Hashable(param.layout.GetHash()), m_MaxSetsPerPool(param.maxSets), m_Device(param.ctx.Device), m_Layout(&param.layout)
    {
        // Get count of each type of descriptor, this information then becomes our template to create descriptor pool
        const auto& bindings = param.layout.GetBindings();
        std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCounts{};
        for (const auto& b : bindings)
        {
            descriptorTypeCounts[b.descriptorType] += b.descriptorCount;
        }
        m_PoolSizes.resize(descriptorTypeCounts.size());
        
        auto insertIter = m_PoolSizes.begin();
        for (const auto& c : descriptorTypeCounts)
        {
            insertIter->type = c.first;
            insertIter->descriptorCount = c.second * param.maxSets;
            ++insertIter;
        }
    }
    
    DescriptorSetAllocator::~DescriptorSetAllocator()
    {
        for (const auto& f : m_Factories)
        {
            vkDestroyDescriptorPool(m_Device, f.Pool.GetHandle(), nullptr);
        }
    }

    VkDescriptorSet DescriptorSetAllocator::Allocate()
    {
        if (!RequireFactory())
        {
            COUST_CORE_ERROR("Can't create descriptor pool, `VK_NULL_HANDLE` is returned");
            return VK_NULL_HANDLE;
        }
        
        ++ m_Factories[m_CurrentFactoryIdx].CurrentProductCount;
        VkDescriptorSetLayout layout = m_Layout->GetHandle();
        VkDescriptorSetAllocateInfo ai 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_Factories[m_CurrentFactoryIdx].Pool.GetHandle(),
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
        };
        bool succeeded = false;
        VkDescriptorSet set = VK_NULL_HANDLE;
        VK_REPORT(vkAllocateDescriptorSets(m_Device, &ai, &set), succeeded);
        if (!succeeded)
        {
            -- m_Factories[m_CurrentFactoryIdx].CurrentProductCount;
            COUST_CORE_ERROR("Can't allocate descriptor set, `VK_NULL_HANDLE` is returned");
            return VK_NULL_HANDLE;
        }
        return set;
    }

    void DescriptorSetAllocator::Reset()
    {
        m_CurrentFactoryIdx = 0;
        
        for (auto& f : m_Factories)
        {
            f.CurrentProductCount = 0;
            vkResetDescriptorPool(m_Device, f.Pool.GetHandle(), 0);
        }
    }
    
    bool DescriptorSetAllocator::RequireFactory()
    {
        uint32_t searchIdx = m_CurrentFactoryIdx;
        while (true)
        {
            // Searching reaches boundary, create new descriptor pool
            if (m_Factories.size() <= searchIdx)
            {
                VkDescriptorPoolCreateInfo ci 
                {
                    .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
                    // We always reset the entire pool instead of freeing each descriptor individually
                    // And unlike command buffer which is reset to "initial" status when the command buffer it attached to is reset, resetting descriptor pool will implicitly free all the descriptor set attached to it.
                    .flags = 0,
                    .maxSets = m_MaxSetsPerPool,
                    .poolSizeCount = (uint32_t) m_PoolSizes.size(),
                    .pPoolSizes = m_PoolSizes.data(),
                };

                VkDescriptorPool pool = VK_NULL_HANDLE;
                VK_CHECK(vkCreateDescriptorPool(m_Device, &ci, nullptr, &pool));
                
                m_Factories.emplace_back(m_Device, pool);
                break;
            }
            // Or there's still enough capacity to allocate new descriptor set
            else if (m_Factories[searchIdx].CurrentProductCount < m_MaxSetsPerPool)
                break;
            // Capacity of current descriptor pool has depleted, search for next one
            else  
                ++ searchIdx;
        }
        
        m_CurrentFactoryIdx = searchIdx;
        return true;
    }
}
