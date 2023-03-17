#include "pch.h"

#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

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

    DescriptorSetLayout::DescriptorSetLayout(const ConstructParam& param)
        : Base(param.ctx, VK_NULL_HANDLE), Hashable(param.GetHash()), m_Set(param.set)
    {
        if (Construct(param.ctx, param.shaderResources))
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
          m_Bindings(std::move(other.m_Bindings)), 
          m_ResNameToBindingIdx(std::move(other.m_ResNameToBindingIdx))
    {
    }


    DescriptorSetLayout::~DescriptorSetLayout()
    {
        vkDestroyDescriptorSetLayout(m_Ctx.Device, m_Handle, nullptr);
    }

    uint32_t DescriptorSetLayout::GetSetIndex() const { return m_Set; }
    
    const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& DescriptorSetLayout::GetBindings() const { return m_Bindings; }

    bool DescriptorSetLayout::Construct(const Context& ctx, 
                                        const std::vector<ShaderResource>& shaderResources)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindingsVec{};
        bindingsVec.reserve(m_Bindings.size());
        for (const auto& res : shaderResources)
        {
            // skip resources without binding point
            if (res.Type == ShaderResourceType::Input ||
                res.Type == ShaderResourceType::Output ||
                res.Type == ShaderResourceType::PushConstant ||
                res.Type == ShaderResourceType::SpecializationConstant)
                continue;

            if (res.Set != m_Set)
            {
                COUST_CORE_WARN("Required to bind shader resource {} with Set {} Bind {}, but the descriptor set index is {}", res.Name, res.Set, res.Binding, m_Set);
                continue;
            }
            
            VkDescriptorType descriptorType = GetDescriptorType(res.Type, res.UpdateMode);
            VkDescriptorSetLayoutBinding binding
            {
                .binding            = res.Binding,
                .descriptorType     = descriptorType,
                .descriptorCount    = res.ArraySize,
                .stageFlags         = res.Stage,
            };

            
            if (auto iter = m_Bindings.find(res.Binding); iter == m_Bindings.end())
            {
                m_Bindings[res.Binding] = binding;
                bindingsVec.push_back(binding);
                m_ResNameToBindingIdx.emplace(res.Name, res.Binding);
            }
            else 
            {
                COUST_CORE_ERROR("There are different shader resources with the same binding while constructing {}. The conflicting binding is: Name {} -> Set {}, Binding {}", m_DebugName, res.Name, m_Set, res.Binding);
                return false;
            }
        }
        
        VkDescriptorSetLayoutCreateInfo ci 
        {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags              = 0,
            .bindingCount       = (uint32_t) bindingsVec.size(),
            .pBindings          = bindingsVec.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(ctx.Device, &ci, nullptr, &m_Handle));
        return true;
    }

    std::optional<VkDescriptorSetLayoutBinding> DescriptorSetLayout::GetBinding(uint32_t bindingIdx) const
    {
        auto iter = m_Bindings.find(bindingIdx);
        if (iter != m_Bindings.end())
            return iter->second;
        else
        {
            COUST_CORE_WARN("{}: Requesting a non-existent descriptor set binding. The binding is: Set {}, Binding {}", m_DebugName, m_Set, bindingIdx);
            return {};
        }
    }
    
    std::optional<VkDescriptorSetLayoutBinding> DescriptorSetLayout::GetBinding(const std::string& name) const
    {
        auto nameIter = m_ResNameToBindingIdx.find(name);
        if (nameIter != m_ResNameToBindingIdx.end())
        {
            auto bindingIter = m_Bindings.find(nameIter->second);
            if (bindingIter != m_Bindings.end())
                return bindingIter->second;
        }

        COUST_CORE_WARN("{}: Requesting a non-existent descriptor set binding. The binding name is {}", m_DebugName, name);
        return {};
    }
    
    DescriptorSetAllocator::DescriptorSetAllocator(const ConstructParam& param)
        : Hashable(param.GetHash()), m_Ctx(param.ctx), m_Layout(&param.layout), m_MaxSetsPerPool(param.maxSets)
    {
        // Get count of each type of descriptor, this information then becomes our template to create descriptor pool
        const auto& bindings = param.layout.GetBindings();
        std::unordered_map<VkDescriptorType, uint32_t> descriptorTypeCounts{};
        descriptorTypeCounts.reserve(bindings.size());
        for (const auto& b : bindings)
        {
            descriptorTypeCounts[b.second.descriptorType] += b.second.descriptorCount;
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
        Reset();

        for (const auto& f : m_Factories)
        {
            vkDestroyDescriptorPool(m_Ctx.Device, f.Pool.GetHandle(), nullptr);
        }
    }

    const DescriptorSetLayout& DescriptorSetAllocator::GetLayout() const { return *m_Layout; }

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
        VK_REPORT(vkAllocateDescriptorSets(m_Ctx.Device, &ai, &set), succeeded);
        if (!succeeded)
        {
            -- m_Factories[m_CurrentFactoryIdx].CurrentProductCount;
            COUST_CORE_ERROR("Can't allocate descriptor set, `VK_NULL_HANDLE` is returned");
            return VK_NULL_HANDLE;
        }
        m_Products.push_back(set);
        return set;
    }

    void DescriptorSetAllocator::Reset()
    {
        m_CurrentFactoryIdx = 0;
        
        for (auto& f : m_Factories)
        {
            f.CurrentProductCount = 0;
            vkResetDescriptorPool(m_Ctx.Device, f.Pool.GetHandle(), 0);
        }

        // descriptor sets have been implicitly freed by `vkResetDescriptorPool`
        m_Products.clear();
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
                    // And unlike command buffer which is reset to "initial" status when the command buffer it attached to is reset, 
                    // resetting descriptor pool will implicitly free all the descriptor sets attached to it.
                    .flags = 0,
                    .maxSets = m_MaxSetsPerPool,
                    .poolSizeCount = (uint32_t) m_PoolSizes.size(),
                    .pPoolSizes = m_PoolSizes.data(),
                };

                VkDescriptorPool pool = VK_NULL_HANDLE;
                VK_CHECK(vkCreateDescriptorPool(m_Ctx.Device, &ci, nullptr, &pool));
                
                m_Factories.emplace_back(m_Ctx, pool);
                break;
            }
            // Or there's still enough capacity to allocate new descriptor set
            else if (m_Factories[searchIdx].CurrentProductCount < m_MaxSetsPerPool)
                break;
            // Capacity of current descriptor pool is depleted, search for next one
            else  
                ++ searchIdx;
        }
        
        m_CurrentFactoryIdx = searchIdx;
        return true;
    }

    DescriptorSet::DescriptorSet(const ConstructParam& param)
        : Base(param.ctx, VK_NULL_HANDLE),
          Hashable(param.GetHash()),
          m_GPUProerpties(*param.ctx.GPUProperties),
          m_Layout(param.layout),
          m_Allocator(param.allocator),
          m_BufferInfos(param.bufferInfos),
          m_ImageInfos(param.imageInfos)
    {
        m_Handle = m_Allocator.Allocate();
        if (m_Handle != VK_NULL_HANDLE)
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, nullptr);
            else
                COUST_CORE_WARN("Descriptor set created without debug name");

            Prepare();
        }
    }

    DescriptorSet::DescriptorSet(DescriptorSet&& other) noexcept
        : Base(std::forward<Base>(other)),
          Hashable(std::forward<Hashable>(other)),
          m_GPUProerpties(other.m_GPUProerpties),
          m_Layout(other.m_Layout),
          m_Allocator(other.m_Allocator),
          m_BufferInfos(std::move(other.m_BufferInfos)),
          m_ImageInfos(std::move(other.m_ImageInfos)),
          m_Writes(std::move(other.m_Writes)),
          m_AppliedWrites(std::move(other.m_AppliedWrites))
    {
    }

    void DescriptorSet::Reset(const std::optional<std::vector<BoundArray<Buffer>>>& bufferInfos,
                              const std::optional<std::vector<BoundArray<Image>>>& imageInfos)
    {
        bool boundInfoUpdated = false;
        if (bufferInfos.has_value())
        {
            m_BufferInfos = bufferInfos.value();
            boundInfoUpdated = true;
        }
        if (imageInfos.has_value())
        {
            m_ImageInfos = imageInfos.value();
            boundInfoUpdated = true;
        }

        if (!boundInfoUpdated)
            COUST_CORE_WARN("Reset descriptor set without specifying new buffer / image info");
        
        m_Writes.clear();
        m_AppliedWrites.clear();
        
        Prepare();
    }

    void DescriptorSet::ApplyWrite(uint32_t bindingsToUpdateMask)
    {
        VkWriteDescriptorSet writeNotYetApplied[sizeof(bindingsToUpdateMask)];
        uint32_t curIdx = 0;
        for (uint32_t i = 0; i < m_Writes.size(); ++ i)
        {
            if (i >= sizeof(bindingsToUpdateMask))
            {
                COUST_CORE_ERROR("There're more than {} binding slots, the type of binding mask should upscale");
                continue;
            }
            if ((1 << i) & bindingsToUpdateMask)
            {
                if (!HasBeenApplied(m_Writes[i]))
                {
                    writeNotYetApplied[curIdx++] = m_Writes[i];
                    // record this write
                    size_t hash = Hash::HashFn<VkWriteDescriptorSet>{}(m_Writes[i]);
                    m_AppliedWrites[m_Writes[i].dstBinding] = hash;
                }
            }
        }

        if (curIdx != 0)
            vkUpdateDescriptorSets(m_Ctx.Device, curIdx, writeNotYetApplied, 0, nullptr);
    }

    void DescriptorSet::ApplyWrite(bool overwrite)
    {
        if (overwrite)
        {
            vkUpdateDescriptorSets(m_Ctx.Device, ToU32(m_Writes.size()), m_Writes.data(), 0, nullptr);
            for (size_t i = 0; i < m_Writes.size(); ++i)
            {
                size_t hash = Hash::HashFn<VkWriteDescriptorSet>{}(m_Writes[i]);
                m_AppliedWrites[m_Writes[i].dstBinding] = hash;
            }
        }
        else
        {
            std::vector<VkWriteDescriptorSet> writeNotYetApplied{};
            writeNotYetApplied.reserve(m_Writes.size());
            for (size_t i = 0; i < m_Writes.size(); ++i)
            {
                // not yet been applied
                if (!HasBeenApplied(m_Writes[i]))
                {
                    writeNotYetApplied.push_back(m_Writes[i]);
                    // record this write
                    size_t hash = Hash::HashFn<VkWriteDescriptorSet>{}(m_Writes[i]);
                    m_AppliedWrites[m_Writes[i].dstBinding] = hash;
                }
            }
            if (!writeNotYetApplied.empty())
                vkUpdateDescriptorSets(m_Ctx.Device, ToU32(writeNotYetApplied.size()), writeNotYetApplied.data(), 0, nullptr);
        }
    }

    const DescriptorSetLayout& DescriptorSet::GetLayout() const { return m_Layout; }

    const std::vector<BoundArray<Buffer>>& DescriptorSet::GetBufferInfo() const { return m_BufferInfos; }

    const std::vector<BoundArray<Image>>& DescriptorSet::GetImageInfo() const { return m_ImageInfos; }

    void DescriptorSet::Prepare()
    {
        if (!m_Writes.empty())
        {
            COUST_CORE_WARN("Can't prepare a descriptor set with a fulfilled write operation vectors.");
            return;
        }

        for (auto& arr : m_BufferInfos)
        {
            uint32_t bindingIdx = arr.bindingIdx;
            std::vector<BoundElement<Buffer>>& buffers = arr.elements;
            if (std::optional<VkDescriptorSetLayoutBinding> bindingInfo = m_Layout.GetBinding(bindingIdx); bindingInfo.has_value())
            {
                for (auto& b : buffers)
                {
                    // clamp the binding range to the GPU limit
                    VkDeviceSize clampedRange = b.range;
                    if (uint32_t uniformBufferRangeLimit = m_GPUProerpties.limits.maxUniformBufferRange;
                        (bindingInfo->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER ||
                         bindingInfo->descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC) &&
                        clampedRange > uniformBufferRangeLimit )
                    {
                        COUST_CORE_WARN("The range (which is {}) of uniform buffer (Set {}, Binding {}) to bind exceeds GPU limit (which is {})", 
                            clampedRange, m_Layout.GetSetIndex(), bindingIdx, uniformBufferRangeLimit);
                        clampedRange = uniformBufferRangeLimit;
                    }
                    else if (uint32_t storageBufferRangeLimit = m_GPUProerpties.limits.maxStorageBufferRange;
                             (bindingInfo->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER ||
                              bindingInfo->descriptorType == VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC) &&
                             clampedRange > storageBufferRangeLimit)
                    {
                        COUST_CORE_WARN("The range (which is {}) of storage buffer (Set {}, Binding {}) to bind exceeds GPU limit (which is {})", 
                            clampedRange, m_Layout.GetSetIndex(), bindingIdx, storageBufferRangeLimit);
                        clampedRange = storageBufferRangeLimit;
                    }
                    b.range = clampedRange;

                    // https://en.cppreference.com/w/cpp/language/data_members#Standard-layout
                    // A pointer to an object of standard-layout class type can be reinterpret_cast to pointer to its first non-static non-bitfield data member 
                    // (if it has non-static data members) or otherwise any of its base class subobjects (if it has any), and vice versa. 
                    // In other words, padding is not allowed before the first data member of a standard-layout type.
                    static_assert(std::is_standard_layout<BoundElement<Buffer>>::value, "");
                    VkWriteDescriptorSet write
                    {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = m_Handle,
                        .dstBinding = bindingInfo->binding,
                        // record one element at a time
                        .dstArrayElement = b.dstArrayIdx,
                        .descriptorCount = 1,
                        .descriptorType = bindingInfo->descriptorType,
                        // `BoundElement<*>` is a standard-layout class type so we can convert its pointer directly to `VkDescriptorBufferInfo*`
                        .pBufferInfo = (const VkDescriptorBufferInfo*) &b,
                    };
                    m_Writes.push_back(write);
                }
            }
            else
                COUST_CORE_WARN("Buffer Binding {} isn't used at descriptor Set {}", bindingIdx, m_Layout.GetSetIndex());
        }

        for (auto& arr : m_ImageInfos)
        {
            uint32_t bindingIdx = arr.bindingIdx;
            std::vector<BoundElement<Image>>& images = arr.elements;
            if (std::optional<VkDescriptorSetLayoutBinding> bindingInfo = m_Layout.GetBinding(bindingIdx); bindingInfo.has_value())
            {
                for (auto& i : images)
                {
                    static_assert(std::is_standard_layout<BoundElement<Image>>::value, "");
                    VkWriteDescriptorSet write
                    {
                        .sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
                        .dstSet = m_Handle,
                        .dstBinding = bindingInfo->binding,
                        // record one element at a time
                        .dstArrayElement = i.dstArrayIdx,
                        .descriptorCount = 1,
                        .descriptorType = bindingInfo->descriptorType,
                        // `BoundElement<*>` is a standard-layout class type so we can convert its pointer directly to `VkDescriptorBufferInfo*`
                        .pImageInfo = (const VkDescriptorImageInfo*) &i,
                    };
                    m_Writes.push_back(write);
                }
            }
            else
                COUST_CORE_WARN("Image Binding {} isn't used at descriptor Set {}", bindingIdx, m_Layout.GetSetIndex());
        }
    }

    bool DescriptorSet::HasBeenApplied(const VkWriteDescriptorSet& write)
    {
        if (auto iter = m_AppliedWrites.find(write.dstBinding); iter != m_AppliedWrites.end())
        {
            size_t hash = Hash::HashFn<VkWriteDescriptorSet>{}(write);
            return hash == iter->second;
        }

        return false;
    }


    /* Hashes */
    size_t DescriptorSetLayout::ConstructParam::GetHash() const
    {
        size_t h = 0;
        for (const auto& s : shaderModules)
        {
            Hash::Combine(h, *s);
        }
        return h;
    }

    size_t DescriptorSet::ConstructParam::GetHash() const
    {
        size_t h = 0;

        Hash::Combine(h, layout);

        for (const auto& b : bufferInfos)
        {
            Hash::Combine(h, b.bindingIdx);
            for (const auto& e : b.elements)
            {
                Hash::Combine(h, e.dstArrayIdx);
                Hash::Combine(h, *(const VkDescriptorBufferInfo*)(&e));
            }
        }

        for (const auto& i : imageInfos)
        {
            Hash::Combine(h, i.bindingIdx);
            for (const auto& e : i.elements)
            {
                Hash::Combine(h, e.dstArrayIdx);
                Hash::Combine(h, *(const VkDescriptorImageInfo*)(&e));
            }
        }

        return h;
    }

    size_t DescriptorSetAllocator::ConstructParam::GetHash() const
    {
        return layout.GetHash();
    }
    /* Hashes */
}


