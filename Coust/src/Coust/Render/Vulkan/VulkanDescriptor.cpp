#include "pch.h"

#include "Coust/Render/Vulkan/VulkanDescriptor.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanShader.h"
#include "Coust/Render/Vulkan/VulkanMemory.h"

#include "Coust/Utils/Hash.h"

namespace Coust::Render::VK
{
    inline VkDescriptorType GetDescriptorType(ShaderResourceType type, ShaderResourceUpdateMode mode) noexcept
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
                COUST_CORE_ASSERT(false, "Can't find suitable `VkDescriptorType` for {}", ToString(type));
                return VK_DESCRIPTOR_TYPE_MAX_ENUM;
        }
    }

    DescriptorSetLayout::DescriptorSetLayout(const ConstructParam& param)  noexcept
        : Base(param.ctx, VK_NULL_HANDLE), Hashable(param.GetHash()), m_Set(param.set)
    {
        std::vector<VkDescriptorSetLayoutBinding> bindingsVec{};
        bindingsVec.reserve(m_Bindings.size());
        for (const auto& res : param.shaderResources)
        {
            // skip resources without binding point
            if (res.Type == ShaderResourceType::Input ||
                res.Type == ShaderResourceType::Output ||
                res.Type == ShaderResourceType::PushConstant ||
                res.Type == ShaderResourceType::SpecializationConstant)
                continue;

            COUST_CORE_ASSERT(res.Set == m_Set,
                "Required to bind shader resource {} with Set {} Bind {}, but the descriptor set index is {}", res.Name, res.Set, res.Binding, m_Set);
            
            // if (res.UpdateMode == ShaderResourceUpdateMode::UpdateAfterBind)
            //     m_RequiredPoolFlags |= VK_DESCRIPTOR_POOL_CREATE_UPDATE_AFTER_BIND_BIT;
            VkDescriptorType descriptorType = GetDescriptorType(res.Type, res.UpdateMode);
            VkDescriptorSetLayoutBinding binding
            {
                .binding            = res.Binding,
                .descriptorType     = descriptorType,
                .descriptorCount    = res.ArraySize,
                .stageFlags         = res.Stage,
            };
            auto iter = m_Bindings.find(res.Binding);
            COUST_CORE_PANIC_IF( iter != m_Bindings.end(), 
                "There are different shader resources with the same binding while constructing {}. The conflicting binding is: Name {} -> Set {}, Binding {}", 
                m_DebugName, res.Name, m_Set, res.Binding);
            m_Bindings[res.Binding] = binding;
            bindingsVec.push_back(binding);
            m_ResNameToBindingIdx.emplace(res.Name, res.Binding);
        }
        
        VkDescriptorSetLayoutCreateInfo ci 
        {
            .sType              = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
            .flags              = 0,
            .bindingCount       = (uint32_t) bindingsVec.size(),
            .pBindings          = bindingsVec.data(),
        };
        VK_CHECK(vkCreateDescriptorSetLayout(param.ctx.Device, &ci, nullptr, &m_Handle), "Can't create descriptor set layout {} {}", param.scopeName, param.dedicatedName);

#ifndef COUST_FULL_RELEASE
        if (param.dedicatedName)
            SetDedicatedDebugName(param.dedicatedName);
        else if (param.scopeName)
            SetDefaultDebugName(param.scopeName, nullptr);
        else
            COUST_CORE_WARN("Descriptor set layout created without a debug name");
#endif
    }

    DescriptorSetLayout::DescriptorSetLayout(DescriptorSetLayout&& other) noexcept
        : Base(std::forward<Base>(other)), Hashable(std::forward<Hashable>(other)),
          m_Set(other.m_Set), 
          m_Bindings(std::move(other.m_Bindings)), 
          m_ResNameToBindingIdx(std::move(other.m_ResNameToBindingIdx))
    {
    }


    DescriptorSetLayout::~DescriptorSetLayout() noexcept
    {
        vkDestroyDescriptorSetLayout(m_Ctx.Device, m_Handle, nullptr);
    }

    uint32_t DescriptorSetLayout::GetSetIndex() const noexcept { return m_Set; }
    
    const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& DescriptorSetLayout::GetBindings() const noexcept { return m_Bindings; }

    VkDescriptorSetLayoutBinding DescriptorSetLayout::GetBinding(uint32_t bindingIdx) const noexcept
    {
        auto iter = m_Bindings.find(bindingIdx);
        COUST_CORE_PANIC_IF(iter == m_Bindings.end(), "{}: Requesting a non-existent descriptor set binding. The binding is: Set {}, Binding {}", m_DebugName, m_Set, bindingIdx);
        return iter->second;
    }
    
    VkDescriptorSetLayoutBinding DescriptorSetLayout::GetBinding(const std::string& name) const noexcept
    {
        auto nameIter = m_ResNameToBindingIdx.find(name);
        if (nameIter != m_ResNameToBindingIdx.end())
        {
            auto bindingIter = m_Bindings.find(nameIter->second);
            if (bindingIter != m_Bindings.end())
                return bindingIter->second;
        }

        COUST_CORE_PANIC_IF(true, "{}: Requesting a non-existent descriptor set binding. The binding name is {}", m_DebugName, name);
        return {};
    }

    VkDescriptorPoolCreateFlags DescriptorSetLayout::GetRequiredPoolFlags() const noexcept { return m_RequiredPoolFlags; }

    DescriptorSet::DescriptorSet(const ConstructParam& param) noexcept
        : Base(*param.ctx, VK_NULL_HANDLE),
          Hashable(param.GetHash()),
          m_GPUProerpties(*param.ctx->GPUProperties),
          m_Layout(param.allocator->GetLayout()),
          m_Allocator(*param.allocator),
          m_BufferInfos(param.bufferInfos),
          m_ImageInfos(param.imageInfos),
          m_SetIdx(param.setIndex)
    {
        m_Handle = m_Allocator.Allocate();
#ifndef COUST_FULL_RELEASE
        if (param.dedicatedName)
            SetDedicatedDebugName(param.dedicatedName);
        else if (param.scopeName)
            SetDefaultDebugName(param.scopeName, nullptr);
#endif
        Prepare();
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

    DescriptorSet::~DescriptorSet() noexcept
    {
        if (m_Handle != VK_NULL_HANDLE)
            m_Allocator.Release(m_Handle);
    }

    // void DescriptorSet::Reset(const std::optional<std::vector<BoundArray<Buffer>>>& bufferInfos,
    //                           const std::optional<std::vector<BoundArray<Image>>>& imageInfos)
    // {
    //     if (bufferInfos.has_value())
    //         m_BufferInfos = bufferInfos.value();
    //     else  
    //         m_BufferInfos.clear();

    //     if (imageInfos.has_value())
    //         m_ImageInfos = imageInfos.value();
    //     else
    //          m_ImageInfos.clear();
        
    //     m_Writes.clear();
    //     m_AppliedWrites.clear();
        
    //     Prepare();
    // }

    void DescriptorSet::ApplyWrite(uint32_t bindingsToUpdateMask) noexcept
    {   
        std::vector<VkWriteDescriptorSet> writeNotYetApplied;
        writeNotYetApplied.reserve(sizeof(bindingsToUpdateMask));
        for (uint32_t i = 0; i < m_Writes.size(); ++ i)
        {
            COUST_CORE_ASSERT(i < sizeof(bindingsToUpdateMask), "There're more than {} binding slots, the type of binding mask should upscale", sizeof(bindingsToUpdateMask));
            if ((1 << i) & bindingsToUpdateMask)
            {
                if (!HasBeenApplied(m_Writes[i]))
                {
                    writeNotYetApplied.push_back(m_Writes[i]);
                    // record this write
                    size_t hash = Hash::HashFn<VkWriteDescriptorSet>{}(m_Writes[i]);
                    m_AppliedWrites[m_Writes[i].dstBinding] = hash;
                }
            }
        }

        if (!writeNotYetApplied.empty())
            vkUpdateDescriptorSets(m_Ctx.Device, (uint32_t) writeNotYetApplied.size(), writeNotYetApplied.data(), 0, nullptr);
    }

    void DescriptorSet::ApplyWrite(bool overwrite) noexcept
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

    const DescriptorSetLayout& DescriptorSet::GetLayout() const noexcept { return m_Layout; }

    const std::vector<BoundArray<Buffer>>& DescriptorSet::GetBufferInfo() const noexcept { return m_BufferInfos; }

    const std::vector<BoundArray<Image>>& DescriptorSet::GetImageInfo() const noexcept { return m_ImageInfos; }

    uint32_t DescriptorSet::GetSetIndex() const noexcept { return m_SetIdx; }

    void DescriptorSet::Prepare() noexcept
    {
        if (!m_Writes.empty())
        {
            COUST_CORE_WARN("Can't prepare a descriptor set with the same operation vectors twice.");
            return;
        }

        for (auto& arr : m_BufferInfos)
        {
            if (arr.bindingIdx == INVALID_IDX)
                continue;
            uint32_t bindingIdx = arr.bindingIdx;
            std::vector<BoundElement<Buffer>>& buffers = arr.elements;
            if (std::optional<VkDescriptorSetLayoutBinding> bindingInfo = m_Layout.GetBinding(bindingIdx); bindingInfo.has_value())
            {
                for (auto& b : buffers)
                {
                    if (b.buffer == VK_NULL_HANDLE)
                        continue;

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
                    static_assert(std::is_standard_layout_v<BoundElement<Buffer>>, "");
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
            if (arr.bindingIdx == INVALID_IDX)
                continue;
            uint32_t bindingIdx = arr.bindingIdx;
            std::vector<BoundElement<Image>>& images = arr.elements;
            if (std::optional<VkDescriptorSetLayoutBinding> bindingInfo = m_Layout.GetBinding(bindingIdx); bindingInfo.has_value())
            {
                for (auto& i : images)
                {
                    if (i.imageView == VK_NULL_HANDLE)
                        continue;

                    static_assert(std::is_standard_layout_v<BoundElement<Image>>, "");
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

    bool DescriptorSet::HasBeenApplied(const VkWriteDescriptorSet& write) const noexcept
    {
        if (auto iter = m_AppliedWrites.find(write.dstBinding); iter != m_AppliedWrites.end())
        {
            size_t hash = Hash::HashFn<VkWriteDescriptorSet>{}(write);
            return hash == iter->second;
        }

        return false;
    }
    
    DescriptorSetAllocator::DescriptorSetAllocator(const ConstructParam& param) noexcept
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

    DescriptorSetAllocator::DescriptorSetAllocator(DescriptorSetAllocator&& other) noexcept
        : Hashable(std::forward<Hashable>(other)),
          m_Ctx(other.m_Ctx),
          m_Layout(other.m_Layout),
          m_PoolSizes(std::move(other.m_PoolSizes)),
          m_Factories(std::move(other.m_Factories)),
          m_FreeSets(std::move(other.m_FreeSets)),
          m_MaxSetsPerPool(other.m_MaxSetsPerPool),
          m_CurrentFactoryIdx(other.m_CurrentFactoryIdx)
    {
    }
    
    DescriptorSetAllocator::~DescriptorSetAllocator() noexcept
    {
        Reset();

        for (const auto& f : m_Factories)
        {
            vkDestroyDescriptorPool(m_Ctx.Device, f.Pool.GetHandle(), nullptr);
        }
    }

    const DescriptorSetLayout& DescriptorSetAllocator::GetLayout() const noexcept { return *m_Layout; }

    VkDescriptorSet DescriptorSetAllocator::Allocate() noexcept
    {
        if (!m_FreeSets.empty())
        {
            auto set = m_FreeSets.back();
            m_FreeSets.pop_back();
            return set;
        }

        RequireFactory();
        
        ++ m_Factories[m_CurrentFactoryIdx].CurrentProductCount;
        VkDescriptorSetLayout layout = m_Layout->GetHandle();
        VkDescriptorSetAllocateInfo ai 
        {
            .sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
            .descriptorPool = m_Factories[m_CurrentFactoryIdx].Pool.GetHandle(),
            .descriptorSetCount = 1,
            .pSetLayouts = &layout,
        };
        VkDescriptorSet set = VK_NULL_HANDLE;
        VK_CHECK(vkAllocateDescriptorSets(m_Ctx.Device, &ai, &set), "Can't allocate descriptor set");
        return set;
    }

    void DescriptorSetAllocator::Release(VkDescriptorSet set) noexcept
    {
        m_FreeSets.push_back(set);
    }

    void DescriptorSetAllocator::Reset() noexcept
    {
        m_CurrentFactoryIdx = 0;
        
        for (auto& f : m_Factories)
        {
            f.CurrentProductCount = 0;
            vkResetDescriptorPool(m_Ctx.Device, f.Pool.GetHandle(), 0);
        }

        // descriptor sets have been implicitly freed by `vkResetDescriptorPool`
        m_FreeSets.clear();
    }

    void DescriptorSetAllocator::FillEmptyDescriptorSetConstructParam(DescriptorSet::ConstructParam& param) const noexcept
    {
        param.allocator = (DescriptorSetAllocator*) this;
        param.setIndex = m_Layout->GetSetIndex();

        uint32_t biggestBindingIdx = 0;
        for (const auto& pair: m_Layout->GetBindings())
        {
            biggestBindingIdx = std::max(pair.first, biggestBindingIdx);
        }
		// To avoid too much searching when binding resources later, we fill in all possible bindings;
        param.bufferInfos.resize(biggestBindingIdx + 1);
        param.imageInfos.resize(biggestBindingIdx + 1);
        for (const auto& pair: m_Layout->GetBindings())
        {
            uint32_t bindIdx = pair.first;
            const VkDescriptorSetLayoutBinding& binding = pair.second;
            switch (binding.descriptorType)
            {
                case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
                case VK_DESCRIPTOR_TYPE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
                case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
                    param.imageInfos[bindIdx].bindingIdx = bindIdx;
                    param.imageInfos[bindIdx].elements.resize(binding.descriptorCount);
                    break;
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: 
                case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: 
                case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
                    param.bufferInfos[bindIdx].bindingIdx = bindIdx;
                    param.bufferInfos[bindIdx].elements.resize(binding.descriptorCount);
                    break;
                default:
                    break;
            }
        }
    }
    
    void DescriptorSetAllocator::RequireFactory() noexcept
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
                    // We always reset the entire pool instead of freeing each descriptor set individually
                    // And unlike command buffer which is reset to "initial" status when the command buffer it attached to is reset, 
                    // resetting descriptor pool will implicitly free all the descriptor sets attached to it.
                    .flags = m_Layout->GetRequiredPoolFlags(),
                    .maxSets = m_MaxSetsPerPool,
                    .poolSizeCount = (uint32_t) m_PoolSizes.size(),
                    .pPoolSizes = m_PoolSizes.data(),
                };

                VkDescriptorPool pool = VK_NULL_HANDLE;
                VK_CHECK(vkCreateDescriptorPool(m_Ctx.Device, &ci, nullptr, &pool), "Can't create descriptor pool");
                
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
    }

    /* Hashes */
    size_t DescriptorSetLayout::ConstructParam::GetHash() const noexcept
    {
        size_t h = 0;
        for (const auto& s : shaderModules)
        {
            Hash::Combine(h, *s);
        }
        return h;
    }

    size_t DescriptorSet::ConstructParam::GetHash() const noexcept
    {
        size_t h = 0;

        Hash::Combine(h, allocator->GetLayout());

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

    size_t DescriptorSetAllocator::ConstructParam::GetHash() const noexcept
    {
        size_t h = 0;
        Hash::Combine(h, layout);
        Hash::Combine(h, maxSets);
        return h;
    }
    /* Hashes */
}


