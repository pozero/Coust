#include "pch.h"

#include "Coust/Render/Vulkan/VulkanMemory.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"

namespace Coust::Render::VK 
{
    inline void GetVmaAllocationConfig(Buffer::Usage usage, VkBufferUsageFlags& out_BufferFlags, VmaMemoryUsage& out_MemoryUsage, VmaAllocationCreateFlags& out_AllocationFlags)
    {
        switch (usage)
        {
            case Buffer::Usage::GPUOnly:
                out_MemoryUsage = VMA_MEMORY_USAGE_AUTO;
                out_AllocationFlags |= VMA_ALLOCATION_CREATE_DEDICATED_MEMORY_BIT;
                return;

            case Buffer::Usage::Staging:
                out_BufferFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                out_MemoryUsage = VMA_MEMORY_USAGE_AUTO;
                out_AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT | 
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
                return;
            case Buffer::Usage::Readback:
                out_BufferFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                out_MemoryUsage = VMA_MEMORY_USAGE_AUTO;
                out_AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
                return;
            case Buffer::Usage::FrequentReadWrite:
                out_BufferFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                out_MemoryUsage = VMA_MEMORY_USAGE_AUTO;
                out_AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
                return;
            default:
                return;
        }
    }

    Buffer::Buffer(ContructParam param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), m_Size(param.size), m_VMAAllocator(param.ctx.VmaAlloc)
    {
        if (Construct(param.ctx, param.bufferFlags, param.usage, param.relatedQueue))
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, ToString<VkBufferUsageFlags, VkBufferUsageFlagBits>(param.bufferFlags).c_str());
            else
                COUST_CORE_WARN("Buffer created without a debug name");
        }
        else  
            m_Handle = VK_NULL_HANDLE;
    }

    Buffer::Buffer(Buffer&& other) noexcept
        : Base(std::forward<Base>(other)), 
          m_Size(other.m_Size),
          m_VMAAllocator(other.m_VMAAllocator),
          m_Domain(other.m_Domain),
          m_UpdateMode(other.m_UpdateMode)
    {
        std::swap(m_Allocation, other.m_Allocation);
        std::swap(m_MappedData, other.m_MappedData);
    }
        
    Buffer::~Buffer()
    {
        if (IsValid())
        {
            m_MappedData = nullptr;
            vmaDestroyBuffer(m_VMAAllocator, m_Handle, m_Allocation);
        }
    }

    bool Buffer::Construct(const Context& ctx, VkBufferUsageFlags bufferFlags, Usage usage, const std::vector<uint32_t>* relatedQueues)
    {
        VmaMemoryUsage vmaMemoryUsage = VMA_MEMORY_USAGE_AUTO;
        VmaAllocationCreateFlags allocationFlags = 0;
        GetVmaAllocationConfig(usage, bufferFlags, vmaMemoryUsage, allocationFlags);

        VkBufferCreateInfo bci 
        {
            .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
            .flags = 0,
            .size = m_Size,
            .usage = bufferFlags,
        };
        if (relatedQueues && relatedQueues->size() > 1)
        {
            bci.sharingMode = VK_SHARING_MODE_CONCURRENT;
            bci.queueFamilyIndexCount = (uint32_t) relatedQueues->size();
            bci.pQueueFamilyIndices = relatedQueues->data();
        }
        
        VmaAllocationCreateInfo aci 
        {
            .flags = allocationFlags,
            .usage = vmaMemoryUsage,
        };
        VmaAllocationInfo ai{};
        
        VK_CHECK(vmaCreateBuffer(ctx.VmaAlloc, &bci, &aci, &m_Handle, &m_Allocation, &ai));
        
        VkMemoryPropertyFlags memPropFlags;
        vmaGetAllocationMemoryProperties(m_VMAAllocator, m_Allocation, &memPropFlags);
        if (memPropFlags & VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT && memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            m_Domain = Domain::HostAndDevice;
        else if (memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            m_Domain = Domain::HostOnly;
        else
            m_Domain = Domain::DeviceOnly;

        if (memPropFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            m_UpdateMode = UpdateMode::AutoFlush;
        
        if (m_Domain != Domain::DeviceOnly && allocationFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
            m_MappedData = (uint8_t*) ai.pMappedData;

        return true;
    }
    
    void Buffer::SetAlwaysFlush(bool shouldAlwaysFlush)
    {
        if (m_UpdateMode == UpdateMode::AutoFlush)
            return;
        m_UpdateMode = shouldAlwaysFlush ? UpdateMode::AlwaysFlush : UpdateMode::FlushOnDemand;
    }

    void Buffer::Flush() const
    {
        vmaFlushAllocation(m_VMAAllocator, m_Allocation, 0, m_Size);
    }

    inline VkImageType GetImageType(VkExtent3D& extent)
    {
        uint32_t dimensionCount = 0;
        if (extent.height >= 1)
            ++ dimensionCount;
        if (extent.width >= 1)
            ++ dimensionCount;
        if (extent.depth > 1)
            ++ dimensionCount;

        switch (dimensionCount)
        {
            case 1:
                return VK_IMAGE_TYPE_1D;
            case 2:
                return VK_IMAGE_TYPE_2D;
            case 3:
                return VK_IMAGE_TYPE_3D;
            default:
                COUST_CORE_WARN("Can't find appropriate image type based on its extent, which has no dimension");
                return VK_IMAGE_TYPE_MAX_ENUM;
        }
    }

    Image::Image(ConstructParam param)
        : Resource(param.ctx.Device, VK_NULL_HANDLE), 
          m_Extent(param.extent),
          m_SubResource{ 0, param.mipLevels, param.arrayLayers },
          m_Format(param.format),
          m_VMAAllocator(param.ctx.VmaAlloc),
          m_ImageUsage(param.imageUsage),
          m_SampleCount(param.samples),
          m_Tiling(param.tiling)
    {
        if (param.mipLevels == 0 || param.arrayLayers == 0)
        {
            COUST_CORE_ERROR("Image should have at least one mip level and one array layer.");
            return;
        }
        m_Type = GetImageType(param.extent);

        // TODO: Now the VmaMemoryUsage and VmaAllocationCreateFlags are fixed. We need some kind of image usage enum class later on.
        if (Construct(param.flags, VMA_MEMORY_USAGE_AUTO, 0, param.initialLayout, param.relatedQueues))
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, ToString<VkImageUsageFlags, VkImageUsageFlagBits>(param.imageUsage).c_str());
            else
                COUST_CORE_WARN("Image created without a debug name");
        }
        else  
            m_Handle = VK_NULL_HANDLE;
    }

    Image::Image(Image&& other) noexcept
        : Base(std::forward<Base>(other)),
          m_ViewsAttached(other.m_ViewsAttached),
          m_Extent(other.m_Extent),
          m_SubResource(other.m_SubResource),
          m_Format(other.m_Format),
          m_VMAAllocator(other.m_VMAAllocator),
          m_Allocation(other.m_Allocation),
          m_ImageUsage(other.m_ImageUsage),
          m_Type(other.m_Type),
          m_SampleCount(other.m_SampleCount),
          m_Tiling(other.m_Tiling)
    {
        other.m_Allocation = VK_NULL_HANDLE;
        for (auto v : m_ViewsAttached)
        {
            v->SetImage(*this);
        }
    }

    bool Image::Construct(VkImageCreateFlags              flags,
                          VmaMemoryUsage                  memoryUsage,
                          VmaAllocationCreateFlags        allocationFlags,
                          VkImageLayout                   initialLayout,
                          const std::vector<uint32_t>*    relatedQueues)
    {
        VkImageCreateInfo ci 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = flags,
            .imageType = m_Type,
            .format = m_Format,
            .extent = m_Extent,
            .mipLevels = m_SubResource.mipLevel,
            .arrayLayers = m_SubResource.arrayLayer,
            .samples = m_SampleCount,
            .tiling = m_Tiling,
            .usage = m_ImageUsage,
            .initialLayout = initialLayout,
        };
        if (relatedQueues && relatedQueues->size() > 1)
        {
            ci.sharingMode = VK_SHARING_MODE_CONCURRENT;
            ci.queueFamilyIndexCount = (uint32_t) relatedQueues->size();
            ci.pQueueFamilyIndices = relatedQueues->data();
        }

        VmaAllocationCreateInfo ai 
        {
            .flags = allocationFlags,
            .usage = memoryUsage,
        };
        // https://gpuopen.com/learn/vulkan-renderpasses/
        // Finally, Vulkan includes the concept of transient attachments. 
        // These are framebuffer attachments that begin in an uninitialized or cleared state at the beginning of a renderpass, 
        // are written by one or more subpasses, consumed by one or more subpasses and are ultimately discarded at the end of the renderpass. 
        // In this scenario, the data in the attachments only lives within the renderpass and never needs to be written to main memory. 
        // Although we��ll still allocate memory for such an attachment, the data may never leave the GPU, instead only ever living in cache. 
        // This saves bandwidth, reduces latency and improves power efficiency.
        if (m_ImageUsage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        {
            ai.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        }

        // reserved for future use
        VmaAllocationInfo info{};

        VK_CHECK(vmaCreateImage(m_VMAAllocator, &ci, &ai, &m_Handle, &m_Allocation, &info));
        return true;
    }

    Image::~Image()
    {
        if (IsValid())
            vmaDestroyImage(m_VMAAllocator, m_Handle, m_Allocation);
        for (auto v : m_ViewsAttached)
            v->InValidate();
    }

    ImageView::ImageView(ConstructParam param)
        : Base(param.ctx.Device, VK_NULL_HANDLE), 
          m_Format(param.format), 
          m_SubresourceRange{ 
            param.aspectMask,
            param.baseMipLevel,
            param.levelCount,
            param.baseArrayLayer,
            param.layerCount
          },
          m_Image(&param.image)
    {
        if (param.layerCount == 0 || param.levelCount == 0)
        {
            COUST_CORE_ERROR("Can't create image view with 0 mipmap level count or 0 array layer count");
            return;
        }

        // use the format of the image it attached to 
        if (param.format == VK_FORMAT_UNDEFINED)
            m_Format = param.image.GetFormat();

        // if the value of `levelCount` or `layerCount` is `VK_REMAINING_MIP_LEVELS` or `VK_REMAINING_ARRAY_LAYERS` (both are ~0u), calculate the actual count
        m_SubresourceRange.levelCount = param.levelCount == VK_REMAINING_MIP_LEVELS ? 
            param.image.GetSubResource().mipLevel - param.baseMipLevel : 
            param.levelCount;
        m_SubresourceRange.layerCount = param.layerCount == VK_REMAINING_ARRAY_LAYERS ? 
            param.image.GetSubResource().arrayLayer - param.baseArrayLayer : 
            param.layerCount;

        if (Construct(param.ctx, param.type))
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, param.image.m_DebugName.c_str());
            else
                COUST_CORE_WARN("Image view created without a debug name");

            m_IsValid = true;

            m_Image->AddView(this);
        }
        else
            m_Handle = VK_NULL_HANDLE;
    }

    ImageView::~ImageView()
    {
        if (m_Image)
            m_Image->EraseView(this);

        if (m_Handle != VK_NULL_HANDLE)
        {
            m_IsValid = false;
            vkDestroyImageView(m_Device, m_Handle, nullptr);
        }
    }

    ImageView::ImageView(ImageView&& other)
        : Base(std::forward<Base>(other)),
        m_Format(other.m_Format),
        m_SubresourceRange(other.m_SubresourceRange)
    {
        std::swap(m_Image, other.m_Image);
        if (other.m_IsValid)
        {
            m_Image->EraseView(&other);
            other.m_IsValid = false;
            m_IsValid = true;
            m_Image->AddView(this);
        }
    }

    bool ImageView::Construct(const Context& ctx, VkImageViewType type)
    {
        VkImageViewCreateInfo ci
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .flags = 0,
            .image = m_Image->GetHandle(),
            .viewType = type,
            .format = m_Format,
            .components = 
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = m_SubresourceRange,
        };

        VK_CHECK(vkCreateImageView(m_Device, &ci, nullptr, &m_Handle));

        return true;
    }
}