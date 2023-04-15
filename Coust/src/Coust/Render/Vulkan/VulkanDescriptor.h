#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

namespace Coust::Render::VK
{
    struct ShaderResource;
    class ShaderModule;
    class Buffer;
    class Image;

    class DescriptorSetLayout;
    class DescriptorSet;
    class DescriptorSetAllocator;

    // Structure containing info about a single element in a binding slot
    template<typename T>
    struct BoundElement;

    template <>
    struct BoundElement<Buffer>
    {
        VkBuffer buffer = VK_NULL_HANDLE;
        VkDeviceSize offset = 0;
        VkDeviceSize range = 0;
        uint32_t dstArrayIdx = INVALID_IDX;
    };

    template <>
    struct BoundElement<Image>
    {
        VkSampler sampler = VK_NULL_HANDLE;
        VkImageView imageView = VK_NULL_HANDLE;
        VkImageLayout imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        uint32_t dstArrayIdx = INVALID_IDX;
    };

    // Structure containing info about a object or array in a binding slot
    template <typename T>
    struct BoundArray
    {
        std::vector<BoundElement<T>> elements;
        uint32_t bindingIdx = INVALID_IDX;
    };

    // Usually, this class is managed by the pipeline layout
    class DescriptorSetLayout : public Resource<VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>,
                                public Hashable
    {
    public:
        using Base = Resource<VkDescriptorSetLayout, VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT>;
    
    public:
        struct ConstructParam
        {
            const Context&                                      ctx;
            uint32_t                                            set;                // Descriptor set number. Usually a unique descriptor set layout is corresponding to a unique set number, so we store it in the layout for convenience.
            size_t                                              shaderModulesCount;
            const ShaderModule* const *                         shaderModules;      // The correspondent shader modules. Passed for hashing.
            size_t                                              shaderResourcesCount;
            const ShaderResource*                               shaderResources;    // Shader resources from SPIR-V reflection contains binding information for construction.
            const char*                                         scopeName = nullptr;
            const char*                                         dedicatedName = nullptr;

            size_t GetHash() const noexcept;
        };
        explicit DescriptorSetLayout(const ConstructParam& param) noexcept;

        ~DescriptorSetLayout() noexcept;
        
        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
        
        DescriptorSetLayout() = delete;
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) = delete;
        
        uint32_t GetSetIndex() const noexcept;
        
        const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& GetBindings() const noexcept;
        
        VkDescriptorSetLayoutBinding GetBinding(uint32_t bindingIdx) const noexcept;

        VkDescriptorSetLayoutBinding GetBinding(const std::string& name) const noexcept;

        VkDescriptorPoolCreateFlags GetRequiredPoolFlags() const noexcept;
        
    private:
        uint32_t m_Set;
        
        // Binding index -> binding info
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings{};

        // shader resource name -> binding index.
        std::unordered_map<std::string, uint32_t> m_ResNameToBindingIdx{};

        VkDescriptorPoolCreateFlags m_RequiredPoolFlags = 0;
    };
    
    class DescriptorSet : public Resource<VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET>,
                          public Hashable
    {
    public:
        using Base = Resource<VkDescriptorSet, VK_OBJECT_TYPE_DESCRIPTOR_SET>;
        
    public:
        struct ConstructParam 
        {
            const Context*                                                  ctx = nullptr;
            DescriptorSetAllocator*                                         allocator;      // Free list of descriptor pool, it contains the descriptor layout and MANAGES the lifecycle of this object
            std::vector<BoundArray<Buffer>>                                 bufferInfos;    // Bound buffer infos, they're bound in arrays
            std::vector<BoundArray<Image>>                                  imageInfos;     // Bound image infos, they're bound in arrays
            uint32_t                                                        setIndex = INVALID_IDX;
            const char*                                                     dedicatedName = nullptr;
            const char*                                                     scopeName = nullptr;

            size_t GetHash() const noexcept;
        };
        explicit DescriptorSet(const ConstructParam& param) noexcept;

        // Lifecycle of descriptor set is managed by descriptor set allocator, so the descructor will release the handle back to its allocator.
        ~DescriptorSet() noexcept;

        DescriptorSet(DescriptorSet&& other) noexcept;

        DescriptorSet() = delete;
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(DescriptorSet&&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;

        // Reset will change the information of bound buffers and images, which will invalidate our hash... 
        // void Reset(const std::optional<std::vector<BoundArray<Buffer>>>& bufferInfos, 
        //            const std::optional<std::vector<BoundArray<Image>>>& imageInfos);
        
        // Flush the write operations in the spcified bingding slot
        void ApplyWrite(uint32_t bindingsToUpdateMask) noexcept;

        // Apply all pending write operations.
        // If overwrite is true, then it will apply all write operations in `m_Writes` without checking if it has been applied (stored in `m_AppliedWrites` in other words)
        void ApplyWrite(bool overwrite = false) noexcept;

        const DescriptorSetLayout& GetLayout() const noexcept;

        const std::vector<BoundArray<Buffer>>& GetBufferInfo() const noexcept;

        const std::vector<BoundArray<Image>>& GetImageInfo() const noexcept;

        uint32_t GetSetIndex() const noexcept;

    private:
        // Fill up the `m_Writes` according to `m_BufferInfos` and `m_ImageInfos`
        void Prepare() noexcept;

        bool HasBeenApplied(const VkWriteDescriptorSet& write) const noexcept;

    private:
        const VkPhysicalDeviceProperties& m_GPUProerpties;

        const DescriptorSetLayout& m_Layout;

        DescriptorSetAllocator& m_Allocator;

        std::vector<BoundArray<Buffer>> m_BufferInfos;

        std::vector<BoundArray<Image>> m_ImageInfos;

        // The write operations to this descriptor set
        std::vector<VkWriteDescriptorSet> m_Writes;

        // binding index -> HASH of `VkWriteDescriptorSet`. This map keeps track of the already written bindings to prevent a double write
        std::unordered_map<uint32_t, size_t> m_AppliedWrites;

        uint32_t m_SetIdx;
    };
    
    // A free list of descriptor pool, perform on-demand allocation of descriptor set. 
    class DescriptorSetAllocator : public Hashable
    {
    public:
        static constexpr uint32_t MAX_SET_PER_POOL = 16;

    public:
        struct ConstructParam
        {
            const Context&                  ctx;
            const DescriptorSetLayout&      layout;
            uint32_t                        maxSets = MAX_SET_PER_POOL;

            size_t GetHash() const noexcept;
        };
        explicit DescriptorSetAllocator(const ConstructParam& param) noexcept;
        
        DescriptorSetAllocator(DescriptorSetAllocator&& other) noexcept;

        ~DescriptorSetAllocator() noexcept;
        
        DescriptorSetAllocator() = delete;
        DescriptorSetAllocator(const DescriptorSetAllocator&) = delete;
        DescriptorSetAllocator& operator=(DescriptorSetAllocator&&) = delete;
        DescriptorSetAllocator& operator=(const DescriptorSetAllocator&) = delete;
        
        const DescriptorSetLayout& GetLayout() const noexcept;
        
        // If we have free descriptor set returned by `DescriptorSet` then use it, otherwise try to allocate a new one.
        VkDescriptorSet Allocate() noexcept;
        
        // Reset the whole pools of descriptor pool, and also recycle all the created descriptor set implicitly.
        void Reset() noexcept;

        // Fill in empty std::vector<BoundArray<*>> in construct parameter. It's handy in constructing descriptor set.
        void FillEmptyDescriptorSetConstructParam(DescriptorSet::ConstructParam& param) const noexcept;
        
    private:
        // Find if there's any available pool *AFTER* `m_CurrentFactoryIdx`, if there is, then only update `m_CurrentFactoryIdx`, 
        // if there isn't, update `m_CurrentFactoryIdx` and allocate a new descriptor pool
        void RequireFactory() noexcept;

        // Accept the handle previously allocated by it.
        friend DescriptorSet::~DescriptorSet() noexcept;
        void Release(VkDescriptorSet set) noexcept;

    private:
        struct Factory 
        {
            Resource<VkDescriptorPool, VK_OBJECT_TYPE_DESCRIPTOR_POOL> Pool;
            uint32_t CurrentProductCount = 0;
            
            Factory(const Context& ctx, VkDescriptorPool handle)
                : Pool(ctx, handle)
            {
                Pool.SetDefaultDebugName("VK::DescriptorAllocator", nullptr);
            }
            Factory() = delete;
        };
        
    private:
        const Context& m_Ctx;
        
        const DescriptorSetLayout* m_Layout;

        std::vector<VkDescriptorPoolSize> m_PoolSizes;
        
        std::vector<Factory> m_Factories;

        std::vector<VkDescriptorSet> m_FreeSets;
        
        uint32_t m_MaxSetsPerPool;
        
        uint32_t m_CurrentFactoryIdx = 0;
        
    };
}