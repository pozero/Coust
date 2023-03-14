#pragma once

#include "Coust/Render/Vulkan/VulkanContext.h"

#include <unordered_set>

namespace Coust::Render::VK 
{
    class Buffer;
    class Image;
    class ImageView;
    class StagePool;

    class Buffer : public Resource<VkBuffer, VK_OBJECT_TYPE_BUFFER>
    {
    public:
        using Base = Resource<VkBuffer, VK_OBJECT_TYPE_BUFFER>;

        // The classification comes from the VMA official manual: https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/usage_patterns.html
        
        enum class Domain 
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
            VkBufferUsageFlags                  bufferFlags;            // Specify usage for optimal memory allocation
            Usage                               usage;                  // Tell vulkan memory allocator how to allocate the memory bound to this buffer
            const std::vector<uint32_t>*        relatedQueue = nullptr; // Optional. The queues that will get access to this buffer
            const char*                         scopeName = nullptr;
            const char*                         dedicatedName = nullptr;
        };
        Buffer(ConstructParam param);

        ~Buffer();

        Buffer(Buffer&& other) noexcept;
        
        Buffer() = delete;
        Buffer(const Buffer&) = delete;
        Buffer& operator=(Buffer&&) = delete;
        Buffer& operator=(const Buffer&) = delete;

        void SetAlwaysFlush(bool shouldAlwaysFlush);
        
        /**
         * @brief Flush memory if it's `HOST_VISIBLE` but not `HOST_COHERENT`
         *        Note: Also, Windows drivers from all 3 PC GPU vendors (AMD, Intel, NVIDIA) currently provide HOST_COHERENT flag on all memory types that are HOST_VISIBLE, 
         *              so on PC you may not need to bother.
         *        https://gpuopen-librariesandsdks.github.io/VulkanMemoryAllocator/html/memory_mapping.html#memory_mapping_persistently_mapped_memory
         */
        void Flush() const;
        
        // Update func for host-visible buffer
        template<typename T>
        void Update(const T& obj, size_t offset = 0)
        {
            update(&obj, 1, offset);
        }
        
        template<typename T>
        void Update(const std::vector<T>& data, size_t offset = 0)
        {
            update(data.data(), data.size(), offset);
        }
        
        template<typename T>
        void Update(const T* data, size_t count, size_t offset = 0)
        {
            if (m_Domain != Domain::DeviceOnly)
            {
                std::memcpy(m_MappedData + offset, data, count * sizeof(T));
                if (m_UpdateMode == UpdateMode::AlwaysFlush)
                    Flush();
            }
            else
                COUST_CORE_WARN("Try to update a memory without `VK_MEMORY_PROPERTY_HOST_VISIBLE`");
        }

        // TODO: All kinds of memory usage the code provides now don't support random access, comment it for now
        // const uint8_t* GetMappedData() const { return m_MappedData; }

		VkDeviceSize GetSize() const;
		
		VmaAllocation GetAllocation() const;

        Domain GetMemoryDomain() const;
        
        bool IsValid() const;
        
    private:
        /**
         * @brief Actual construction happens here
         */
        bool Construct(const Context& ctx, VkBufferUsageFlags bufferFlags, Usage usage, const std::vector<uint32_t>* relatedQueues);

    private:
        VkDeviceSize m_Size;
        
        VmaAllocator m_VMAAllocator;

        VmaAllocation m_Allocation = VK_NULL_HANDLE;

        uint8_t* m_MappedData = nullptr;

        Domain m_Domain;

        UpdateMode m_UpdateMode = UpdateMode::AlwaysFlush;
    };

    class Image : public Resource<VkImage, VK_OBJECT_TYPE_IMAGE>
    {
    public:
        using Base = Resource<VkImage, VK_OBJECT_TYPE_IMAGE>;

    public:
        struct ConstructParam
        {
            const Context&                  ctx;
            VkExtent3D                      extent;
            VkFormat                        format;
            VkImageUsageFlags               imageUsage;
            VkImageCreateFlags              flags = 0;
            uint32_t                        mipLevels = 1;
            uint32_t                        arrayLayers = 1;
            VkSampleCountFlagBits           samples = VK_SAMPLE_COUNT_1_BIT;
            VkImageTiling                   tiling = VK_IMAGE_TILING_OPTIMAL;
            VkImageLayout                   initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            const std::vector<uint32_t>*    relatedQueues = nullptr;
            const char*                     dedicatedName = nullptr;
            const char*                     scopeName = nullptr;
        };
        Image(ConstructParam param);

        ~Image();

        Image(Image&& other) noexcept;

        Image() = delete;
        Image(const Image&) = delete;
        Image& operator=(Image&&) = delete;
        Image& operator=(const Image&) = delete;

        VkExtent3D GetExtent() const;

        VkFormat GetFormat() const;

        VkImageUsageFlags GetUsage() const;

        VkImageType GetType() const;

        VkSampleCountFlagBits GetSampleCount() const;

        VkImageTiling GetTiling() const;

        VkImageSubresource GetSubResource() const;

        VmaAllocation GetAllocation() const;

        const std::unordered_set<ImageView*> GetAttachedView() const;

        bool IsValid() const;

    private:
        friend class ImageView;

        /**
         * @brief Get called when the attached view is destroyed or moved
         * @param view 
        */
        void EraseView(ImageView* view);

        /**
         * @brief Get called when the attached view is constructed or moved
        */
        void AddView(ImageView* view);

    private:
        bool Construct(VkImageCreateFlags              flags,
                       VmaMemoryUsage                  memoryUsage,
                       VmaAllocationCreateFlags        allocationFlags,
                       VkImageLayout                   initialLayout,
                       const std::vector<uint32_t>*    relatedQueues);

    private:
        /**
         * @brief Used to track the image views attached to this image in case it's moved or destructed
        */
        std::unordered_set<ImageView*> m_ViewsAttached{};

        VkExtent3D m_Extent;

        VkImageSubresource m_SubResource;

        VkFormat m_Format;

        VmaAllocator m_VMAAllocator;

        VmaAllocation m_Allocation;

        VkImageUsageFlags m_ImageUsage;

        VkImageType m_Type;

        VkSampleCountFlagBits m_SampleCount;

        VkImageTiling m_Tiling;
    };

    class ImageView : public Resource<VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW>
    {
    public:
        using Base = Resource<VkImageView, VK_OBJECT_TYPE_IMAGE_VIEW>;

    public:
        struct ConstructParam
        {
            const Context&          ctx;
            Image&                  image;
            VkImageViewType         type;
            VkFormat                format = VK_FORMAT_UNDEFINED;
            VkImageAspectFlags      aspectMask;
            uint32_t                baseMipLevel;
            uint32_t                levelCount;
            uint32_t                baseArrayLayer;
            uint32_t                layerCount;
            const char*             dedicatedName = nullptr;
            const char*             scopeName = nullptr;
        };
        ImageView(ConstructParam param);

        ImageView(ImageView&& other) noexcept;

        ~ImageView();

        ImageView() = delete;
        ImageView(const ImageView&) = delete;
        ImageView operator=(ImageView&&) = delete;
        ImageView& operator=(const ImageView&) = delete;

        VkFormat GetFormat() const;

        VkImageSubresourceRange GetSubresourceRange() const;

        const Image* GetImage() const;

        bool IsValid() const;

    private:
        friend class Image;

        /**
         * @brief Get called when the image it attched to is destroyed
         */
        void InValidate();

        /**
         * @brief Get called when the image it attched to is moved
         * 
         * @param image 
         */
        void SetImage(Image& image);

    private:
        /**
         * @brief Actual construction happens here
         * 
         * @param ctx 
         * @param type 
         * @return false if construction failed
         */
        bool Construct(const Context& ctx, VkImageViewType type);

    private:
        VkFormat m_Format;

        VkImageSubresourceRange m_SubresourceRange;

        Image* m_Image;

        bool m_IsValid = false;
    };
}