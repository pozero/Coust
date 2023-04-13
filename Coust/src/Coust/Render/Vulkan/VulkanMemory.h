#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"
#include "Coust/Utils/Hash.h"

#include <map>
#include <unordered_map>

namespace Coust::Render::VK 
{
    class Buffer;
    class Image;
    class StagePool;
        
    enum class MemoryDomain
    {
        HostOnly,
        DeviceOnly,
        // In both cases, the memory can be visible on both host side & device side:
        //      1. On systems with unified memory (e.g. AMD APU or Intel integrated graphics, mobile chips), 
        //         a memory type may be available that is both HOST_VISIBLE (available for mapping) and DEVICE_LOCAL (fast to access from the GPU).
        //      2. Systems with a discrete graphics card and separate video memory may or may not expose a memory type that is both HOST_VISIBLE and DEVICE_LOCAL, 
        //         also known as Base Address Register (BAR). 
        //         Writes performed by the host to that memory go through PCI Express bus. The performance of these writes may be limited, 
        //         but it may be fine, especially on PCIe 4.0, as long as rules of using uncached and write-combined memory are followed - only sequential writes and no reads.
        HostAndDevice,
    };

    class Buffer : public Resource<VkBuffer, VK_OBJECT_TYPE_BUFFER>
    {
    public:
        using Base = Resource<VkBuffer, VK_OBJECT_TYPE_BUFFER>;

        // The classification comes from the VMA official manual: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
        
        enum class Usage
        {
            // Any resources that you frequently write and read on GPU, 
            // e.g. images used as color attachments (aka "render targets"), depth-stencil attachments, 
            // images/buffers used as storage image/buffer (aka "Unordered Access View (UAV)").
            GPUOnly,
            // A "staging" buffer than you want to map and fill from CPU code, then use as a source of transfer to some GPU resource.
            Staging,
            // Buffers for data written by or transferred from the GPU that you want to read back on the CPU, e.g. results of some computations.
            Readback,
            // For resources that you frequently write on CPU via mapped pointer and frequently read on GPU e.g. as a uniform buffer (also called "dynamic")
            // Be careful, frequent read write means it probably be slower than the solution of copying data from staging buffer to a device-local buffer. It needs to be measured.
            FrequentReadWrite,
        };
        
        enum class UpdateMode
        {
            // If the buffer is used for read purpose

            ReadOnly,

            // If the memory is `HOST_VISIBLE` but not `HOST_COHERENT`, then we can choose from them to alleviate cache pressure strategically
        
            // The `Update()` function will always flush after copy
            AlwaysFlush,
            // The `Update()` func won't flush, it's the caller's responsiblity to flush memory
            FlushOnDemand,
        
            // If the memory is `HOST_COHERENT`, then there's no need to bother it
            AutoFlush,
        };
        
    public:
        struct ConstructParam
        {
            const Context&                      ctx;
            VkDeviceSize                        size;
            VkBufferUsageFlags                  bufferFlags;                            // Specify usage for optimal memory allocation
            Usage                               usage;                                  // Tell vulkan memory allocator how to allocate the memory bound to this buffer
            const uint32_t                      relatedQueue[COMMAND_QUEUE_COUNT]       // Optional. The queues that will get access to this buffer
            {
                VK_QUEUE_FAMILY_IGNORED, 
                VK_QUEUE_FAMILY_IGNORED, 
                VK_QUEUE_FAMILY_IGNORED, 
            }; 
            const char*                         scopeName = nullptr;
            const char*                         dedicatedName = nullptr;
        };
        explicit Buffer(const ConstructParam& param) noexcept;

        ~Buffer() noexcept;

        Buffer(Buffer&& other) noexcept;
        
        Buffer() = delete;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(Buffer&&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        void SetAlwaysFlush(bool shouldAlwaysFlush) noexcept;
        
        // Flush memory if it's `HOST_VISIBLE` but not `HOST_COHERENT`
        // Note: Also, Windows drivers from all 3 PC GPU vendors (AMD, Intel, NVIDIA) currently provide HOST_COHERENT flag on all memory types that are HOST_VISIBLE, 
        //       so on PC you may not need to bother.
        // https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html#memory_mapping_persistently_mapped_memory
        void Flush() const noexcept;
        
        template<typename T>
        void Update(StagePool& stagePool, const T& obj, size_t offset = 0) noexcept
        {
            Update(stagePool, (const void*) &obj, sizeof(T), offset);
        }
        
        template<typename T>
        void Update(StagePool& stagePool, const std::vector<T>& data, size_t offset = 0) noexcept
        {
            Update(stagePool, (const void*) data.data(), sizeof(T) * data.size(), offset);
        }
        
        template<typename T>
        void Update(StagePool& stagePool, const T* data, size_t count, size_t offset = 0) noexcept
        {
            Update(stagePool, (const void*) data, count * sizeof(T), offset);
        }

        // the update will check the memory domain and decide whether to use a staging buffer to update its content
        void Update(StagePool& stagePool, const void* data, size_t numBytes, size_t offset = 0) noexcept;

        const uint8_t* GetMappedData() const noexcept;

		VkDeviceSize GetSize() const noexcept;
		
		VmaAllocation GetAllocation() const noexcept;

        MemoryDomain GetMemoryDomain() const noexcept;
        
        bool IsValid() const noexcept;

    private:
        VkDeviceSize m_Size;
        
        VmaAllocator m_VMAAllocator;

        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        uint8_t* m_MappedData = nullptr;

        VkBufferUsageFlags m_Usage;

        MemoryDomain m_Domain;

        UpdateMode m_UpdateMode = UpdateMode::AlwaysFlush;
    };

    class Image : public Resource<VkImage, VK_OBJECT_TYPE_IMAGE>
    {
    public:
        using Base = Resource<VkImage, VK_OBJECT_TYPE_IMAGE>;

        enum class Usage 
        {
            CubeMap,
            Texture2D,
            DepthStencilAttachment,
            ColorAttachment,
            InputAttachment,
        };

        class View : public Resource<VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW>
        {
        public:
            using Base = Resource<VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW>;

        public:
            View() = delete;
            View(const View&) = delete;
            View operator=(View&&) = delete;
            View& operator=(const View&) = delete;

            const Image& GetImage() const noexcept;

            struct ConstructParam
            {
                const Context&          ctx;
                Image&                  image;
                VkImageViewType         type;
                VkImageSubresourceRange subRange;
                const char*             dedicatedName = nullptr;
                const char*             scopeName = nullptr;
            };
            explicit View(const ConstructParam& param) noexcept;

            View(View&& other) noexcept;

            ~View() noexcept;

        private:
            Image& m_Image;
        };

    public:
        // we don't support any kind of 3d image here
        struct ConstructParam_Create
        {
            const Context&                  ctx;
            uint32_t                        width;
            uint32_t                        height;
            VkFormat                        format;
            Usage                           usage;                                          // High level usage
            VkImageUsageFlags               usageFlags = 0;
            VkImageCreateFlags              createFlags = 0;
            uint32_t                        mipLevels = 1;
            VkSampleCountFlagBits           samples = VK_SAMPLE_COUNT_1_BIT;
            VkImageTiling                   tiling = VK_IMAGE_TILING_OPTIMAL;
            VkImageLayout                   initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            const uint32_t                  relatedQueues[COMMAND_QUEUE_COUNT]               // Optional. The queues that will get access to this buffer
            {
                VK_QUEUE_FAMILY_IGNORED, 
                VK_QUEUE_FAMILY_IGNORED, 
                VK_QUEUE_FAMILY_IGNORED, 
            }; 
            const char*                     dedicatedName = nullptr;
            const char*                     scopeName = nullptr;
        };
        explicit Image(const ConstructParam_Create& param) noexcept;

        struct ConstructParam_Wrap
        {
            const Context&              ctx;
            VkImage                     handle;
            uint32_t                    width;
            uint32_t                    height;
            VkFormat                    format;
            VkSampleCountFlagBits       samples;
            const char*                 dedicatedName = nullptr;
            const char*                 scopeName = nullptr;
        };
        explicit Image(const ConstructParam_Wrap& param) noexcept;

        ~Image() noexcept;

        Image(Image&& other) noexcept;

        Image() = delete;
        Image(const Image&) = delete;
        Image& operator=(Image&&) = delete;
        Image& operator=(const Image&) = delete;

        // don't bother 3d image now
        struct UpdateParam 
        {
            VkFormat        dataFormat;
            uint32_t        width;
            uint32_t        height;
            const void*     data;
            uint32_t        dstImageLayer = 0;
            uint32_t        dstImageLayerCount = 1;
            uint32_t        dstImageMipmapLevel = 0;
        };
        void Update(StagePool& stagePool, const UpdateParam& p) noexcept;

        void TransitionLayout(VkCommandBuffer cmdBuf, VkImageLayout newLayout, VkImageSubresourceRange subRange) noexcept;

        VkImageLayout GetLayout(uint32_t layer, uint32_t level) const noexcept;

        // If the class is just a wrapper around a `VkImage` handle, like a swapchain image, then its layout might be changed during renderpass.
        // We can use this method to keep track of the actual layout
        void ChangeLayout(uint32_t layer, uint32_t level, VkImageLayout newLayout) noexcept;

        // return or create the required image view
        const View* GetView(VkImageSubresourceRange subRange) noexcept;

        const View* GetSingleLayerView(VkImageAspectFlags aspect, uint32_t layer, uint32_t mipLevel) noexcept;

        // helper function related to primary subresource range
        VkImageLayout GetPrimaryLayout() const noexcept;
        const View* GetPrimaryView() const noexcept;
        VkImageSubresourceRange GetPrimarySubRange() const noexcept;
        void SetPrimarySubRange(uint32_t minMipmapLevel, uint32_t maxMipmaplevel) noexcept;

        VkExtent2D GetExtent() const noexcept;

        VkFormat GetFormat() const noexcept;

        VmaAllocation GetAllocation() const noexcept;

        VkSampleCountFlagBits GetSampleCount() const noexcept;

        std::shared_ptr<Image> GetMSAAImage() const noexcept;
        void SetMASSImage(std::shared_ptr<Image> massImage) noexcept;

        uint32_t GetMipLevel() const noexcept;

    private:
        std::unordered_map<VkImageSubresourceRange, View, 
            Hash::HashFn<VkImageSubresourceRange>, Hash::EqualFn<VkImageSubresourceRange>>  m_CachedImageView;

        // (layer << 16) | level => image layout of this pair of array layer and mipmap level
        std::map<uint32_t, VkImageLayout> m_SubRangeLayouts;

        std::shared_ptr<Image> m_MSAAImage = nullptr;

        VkExtent3D m_Extent;

        // When used as textures, this member specify the binding range
        VkImageSubresourceRange m_PrimarySubRange;

        VkFormat m_Format;

        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        // the default layout is determined by the type when the image is created
        VkImageLayout m_DefaultLayout;

        // to construct image views
        VkImageViewType m_ViewType;

        VkSampleCountFlagBits m_SampleCount;

        const uint32_t m_MipLevelCount;
    };

    // There's no need to add staging function to the VK::Image class, which is mainly used for device only image
    // So we create another litte helper class here
    class HostImage : public Resource<VkImage, VK_OBJECT_TYPE_IMAGE>
    {
    public:
        using Base = Resource<VkImage, VK_OBJECT_TYPE_IMAGE>;

    public:
        struct ConstructParam
        {
            const Context& ctx;
            VkFormat format;
            uint32_t width;
            uint32_t height;

        };
        explicit HostImage(const ConstructParam& param) noexcept;

        ~HostImage() noexcept;

        void Update(const void* data, size_t size) noexcept;

        VkImageAspectFlags GetAspect() const noexcept;

        VkFormat GetFormat() const noexcept;

        uint32_t GetWidth() const noexcept;

        uint32_t GetHeight() const noexcept;

    private:
        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        void* m_MappedData = nullptr;

        VkFormat m_Format;

        uint32_t m_Width;

        uint32_t m_Height;
    };
}