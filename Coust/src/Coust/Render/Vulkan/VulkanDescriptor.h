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

    /**
     * @brief Structure containing info about a single element in a binding slot
     * @tparam T can only be Buffer, Image
     */
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

    /**
     * @brief Structure containing info about a object or array in a binding slot
     * @tparam T can only be Buffer, Image
     */
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
            const std::vector<ShaderModule*>&                   shaderModules;      // The correspondent shader modules. Passed for hashing.
            const std::vector<ShaderResource>&                  shaderResources;    // Shader resources from SPIR-V reflection contains binding information for construction.
            const char*                                         scopeName = nullptr;
            const char*                                         dedicatedName = nullptr;

            size_t GetHash() const;
        };
        explicit DescriptorSetLayout(const ConstructParam& param);

        ~DescriptorSetLayout();
        
        DescriptorSetLayout(DescriptorSetLayout&& other) noexcept;
        
        DescriptorSetLayout() = delete;
        DescriptorSetLayout(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(const DescriptorSetLayout&) = delete;
        DescriptorSetLayout& operator=(DescriptorSetLayout&& other) = delete;
        
        uint32_t GetSetIndex() const noexcept;
        
        const std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding>& GetBindings() const noexcept;
        
        std::optional<VkDescriptorSetLayoutBinding> GetBinding(uint32_t bindingIdx) const noexcept;

        std::optional<VkDescriptorSetLayoutBinding> GetBinding(const std::string& name) const noexcept;

        VkDescriptorPoolCreateFlags GetRequiredPoolFlags() const noexcept;
        
    private:
        bool Construct(const Context& ctx, const std::vector<ShaderResource>& shaderResources);
        
    private:
        uint32_t m_Set;
        
        /**
         * @brief Binding index -> binding info
         */
        std::unordered_map<uint32_t, VkDescriptorSetLayoutBinding> m_Bindings{};

        /**
         * @brief shader resource name -> binding index.
         */
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

            size_t GetHash() const;
        };
        explicit DescriptorSet(const ConstructParam& param);

        // Lifecycle of descriptor set is managed by descriptor set allocator, so the descructor will release the handle back to its allocator.
        ~DescriptorSet();

        DescriptorSet(DescriptorSet&& other) noexcept;

        DescriptorSet() = delete;
        DescriptorSet(const DescriptorSet&) = delete;
        DescriptorSet& operator=(DescriptorSet&&) = delete;
        DescriptorSet& operator=(const DescriptorSet&) = delete;

        /**
         * @brief Reset the inner state of the descriptor set
         * 
         * @param bufferInfos   Optional. If provided, then the old bufferInfos will be discarded
         * @param imageInfos    Opitonal. If provided, then the old imageInfos will be discarded
         */
        void Reset(const std::optional<std::vector<BoundArray<Buffer>>>& bufferInfos, 
                   const std::optional<std::vector<BoundArray<Image>>>& imageInfos);
        
        /**
         * @brief Flush the write operations in the spcified bingding slot
         * @param bindingsToUpdateMask Mask of binding indices to be updated. It supports at most 32 bindings per set (that's enough, maybe?)
         */
        void ApplyWrite(uint32_t bindingsToUpdateMask);

        /**
         * @brief Apply all pending write operations 
         * 
         * @param overwrite If true, then it will apply all write operations in `m_Writes` without checking if it has been applied (stored in `m_AppliedWrites` in other words)
         */
        void ApplyWrite(bool overwrite = false);

        const DescriptorSetLayout& GetLayout() const noexcept;

        const std::vector<BoundArray<Buffer>>& GetBufferInfo() const noexcept;

        const std::vector<BoundArray<Image>>& GetImageInfo() const noexcept;

        uint32_t GetSetIndex() const noexcept;

    private:
        /**
         * @brief Fill up the `m_Writes` according to `m_BufferInfos` and `m_ImageInfos`
         */
        void Prepare();

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
            const Context&                  ctx;
            const DescriptorSetLayout&      layout;
            uint32_t                        maxSets = MAX_SET_PER_POOL;

            size_t GetHash() const;
        };
        explicit DescriptorSetAllocator(const ConstructParam& param);
        
        DescriptorSetAllocator(DescriptorSetAllocator&& other) noexcept;

        ~DescriptorSetAllocator();
        
        DescriptorSetAllocator() = delete;
        DescriptorSetAllocator(const DescriptorSetAllocator&) = delete;
        DescriptorSetAllocator& operator=(DescriptorSetAllocator&&) = delete;
        DescriptorSetAllocator& operator=(const DescriptorSetAllocator&) = delete;
        
        const DescriptorSetLayout& GetLayout() const;
        
        /**
         * @brief If we have free descriptor set returned by `DescriptorSet` then use it, otherwise try to allocate a new one.
         */
        VkDescriptorSet Allocate();
        
        /**
         * @brief Reset the whole pools of descriptor pool, and also recycle all the created descriptor set implicitly.
         */
        void Reset() noexcept;

        // Fill in empty std::vector<BoundArray<*>> in construct parameter. It's handy in constructing descriptor set.
        void FillEmptyDescriptorSetConstructParam(DescriptorSet::ConstructParam& param) const noexcept;
        
    private:
        /**
         * @brief Find if there's any available pool *AFTER* `m_CurrentFactoryIdx`, if there is, then only update `m_CurrentFactoryIdx`
                  if there isn't, update `m_CurrentFactoryIdx` and allocate a new descriptor pool
         * @return false if the func `vkCreateDescriptorPool` failed
         */
        bool RequireFactory();

        // Accept the handle previously allocated by it.
        friend DescriptorSet::~DescriptorSet();
        void Release(VkDescriptorSet set);

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

        // reserved for future use
        std::vector<VkDescriptorSet> m_FreeSets;
        
        uint32_t m_MaxSetsPerPool;
        
        uint32_t m_CurrentFactoryIdx = 0;
        
    };
}