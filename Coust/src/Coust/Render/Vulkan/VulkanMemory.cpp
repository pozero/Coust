#include "pch.h"

#include "Coust/Render/Vulkan/VulkanMemory.h"
#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Render/Vulkan/VulkanCommand.h"
#include "Coust/Render/Vulkan/StagePool.h"

namespace Coust::Render::VK 
{
    inline void GetVmaAllocationConfig(Buffer::Usage usage, VkBufferUsageFlags& out_BufferFlags, VmaMemoryUsage& out_MemoryUsage, VmaAllocationCreateFlags& out_AllocationFlags)
    {
        switch (usage)
        {
            case Buffer::Usage::GPUOnly:
                out_BufferFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
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
                out_BufferFlags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
                out_BufferFlags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
                out_MemoryUsage = VMA_MEMORY_USAGE_AUTO;
                out_AllocationFlags |= VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT |
                                       VMA_ALLOCATION_CREATE_HOST_ACCESS_ALLOW_TRANSFER_INSTEAD_BIT |
                                       VMA_ALLOCATION_CREATE_MAPPED_BIT;
                return;
            default:
                out_MemoryUsage = VMA_MEMORY_USAGE_AUTO;
                return;
        }
    }

    inline void GetImageConfig(Image::Usage usage, VkImageCreateInfo& CI, VmaAllocationCreateInfo& AI, VkImageViewType& viewType, VkImageLayout& defaultLayout, bool blitable)
    {
        // For the convenience of copying data between image (debug for example), the image can be blitable
        const VkImageUsageFlags blit = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        if (blitable)
            CI.usage |= blit;
        
        // https://gpuopen.com/learn/vulkan-renderpasses/
        // Finally, Vulkan includes the concept of transient attachments. 
        // These are framebuffer attachments that begin in an uninitialized or cleared state at the beginning of a renderpass, 
        // are written by one or more subpasses, consumed by one or more subpasses and are ultimately discarded at the end of the renderpass. 
        // In this scenario, the data in the attachments only lives within the renderpass and never needs to be written to main memory. 
        // Although we'll still allocate memory for such an attachment, the data may never leave the GPU, instead only ever living in cache. 
        // This saves bandwidth, reduces latency and improves power efficiency.
        if (CI.usage & VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT)
        {
            AI.preferredFlags |= VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT;
        }
        
        switch (usage)
        {
            case Image::Usage::CubeMap:
                CI.arrayLayers = 6;
                CI.flags |= VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
                CI.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
                CI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                AI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                viewType = VK_IMAGE_VIEW_TYPE_CUBE;
                defaultLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
                return;
            case Image::Usage::Texture2D:
                CI.arrayLayers = 1;
                CI.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
                CI.usage |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
                AI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                viewType = VK_IMAGE_VIEW_TYPE_2D;
                defaultLayout = VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL_KHR;
                return;
            case Image::Usage::DepthStencilAttachment:
                CI.arrayLayers = 1;
                CI.usage |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
                AI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                viewType = VK_IMAGE_VIEW_TYPE_2D;
                defaultLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
                return;
            case Image::Usage::ColorAttachment:
                CI.arrayLayers = 1;
                CI.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                AI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                viewType = VK_IMAGE_VIEW_TYPE_2D;
                defaultLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
                return;
            case Image::Usage::InputAttachment:
                CI.arrayLayers = 1;
                // Commonly, input attachment is also the color attachment for the previous subpass
                CI.usage |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
                CI.usage |= VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT;
                AI.usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE;
                viewType = VK_IMAGE_VIEW_TYPE_2D;
                defaultLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
                return;
            default: // Image::Type::None
                AI.usage = VMA_MEMORY_USAGE_AUTO;
                return;
        }
    }

    const char* ToString(Image::Usage usage)
    {
        switch (usage)
        {
            case Image::Usage::CubeMap: return "CubeMap";
            case Image::Usage::Texture2D: return "Texture2D";
            case Image::Usage::InputAttachment: return "InputAttachment";
            case Image::Usage::ColorAttachment: return "ColorAttachment";
            case Image::Usage::DepthStencilAttachment: return "DepthStencilAttachment";
            default: return "None";
        }
    }

    Buffer::Buffer(const ConstructParam& param)
        : Base(param.ctx, VK_NULL_HANDLE), 
          m_Size(param.size), 
          m_VMAAllocator(param.ctx.VmaAlloc),
          m_Usage(param.bufferFlags)
    {
        if (Construct(param.ctx, param.bufferFlags, param.usage, param.relatedQueue))
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, ToString<VkBufferUsageFlags, VkBufferUsageFlagBits>(param.bufferFlags).c_str());
            
            if (param.usage == Usage::Readback)
                m_UpdateMode = UpdateMode::ReadOnly;
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

    void Buffer::Update(StagePool& stagePool, const void* data, size_t numBytes, size_t offset)
    {
        // If the buffer is host-visible, then just use memcpy
        if (m_Domain == MemoryDomain::DeviceOnly)
        {
            auto stagingBuf = stagePool.AcquireStagingBuffer(numBytes);
            stagingBuf->Update(stagePool, data, numBytes, offset);
            stagingBuf->Flush();

            VkCommandBuffer cmdBuf = m_Ctx.CmdBufCacheGraphics->Get();
            VkBufferCopy copyInfo 
            {
                .srcOffset = 0,
                .dstOffset = 0,
                .size = numBytes,
            };
            vkCmdCopyBuffer(cmdBuf, stagingBuf->GetHandle(), m_Handle, 1, &copyInfo);

            // Using memory barrier to make sure the data actually reaches the memory before using it
            if (m_Usage & VK_BUFFER_USAGE_VERTEX_BUFFER_BIT ||
                m_Usage & VK_BUFFER_USAGE_INDEX_BUFFER_BIT)
            {
                VkBufferMemoryBarrier2 barrier 
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    // wait for another possible upload
                    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT | 
                        VK_PIPELINE_STAGE_2_VERTEX_INPUT_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT | 
                        VK_ACCESS_2_VERTEX_ATTRIBUTE_READ_BIT | VK_ACCESS_2_INDEX_READ_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = m_Handle,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
                };

                VkDependencyInfo dependency 
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    .memoryBarrierCount = 0,
                    .pMemoryBarriers = nullptr,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &barrier,
                    .imageMemoryBarrierCount = 0,
                    .pImageMemoryBarriers = nullptr,
                };

                vkCmdPipelineBarrier2(cmdBuf, &dependency);
            }

            if (m_Usage & VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT)
            {
                VkBufferMemoryBarrier2 barrier 
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,
                    // wait for another possible upload
                    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT | 
                        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT | 
                        VK_ACCESS_2_UNIFORM_READ_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = m_Handle,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
                };

                VkDependencyInfo dependency 
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    // .dependencyFlags;
                    .memoryBarrierCount = 0,
                    .pMemoryBarriers = nullptr,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &barrier,
                    .imageMemoryBarrierCount = 0,
                    .pImageMemoryBarriers = nullptr,
                };

                vkCmdPipelineBarrier2(cmdBuf, &dependency);
            }

            if (m_Usage & VK_BUFFER_USAGE_STORAGE_BUFFER_BIT)
            {
                VkBufferMemoryBarrier2 barrier 
                {
                    .sType = VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER_2,
                    .srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT,
                    .srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT,

                    // wait for another possible upload of vertex data
                    .dstStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT | 
                        VK_PIPELINE_STAGE_2_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT,
                    .dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT | 
                        VK_ACCESS_2_SHADER_STORAGE_READ_BIT,
                    .srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
                    .buffer = m_Handle,
                    .offset = 0,
                    .size = VK_WHOLE_SIZE,
                };

                VkDependencyInfo dependency 
                {
                    .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
                    // .dependencyFlags;
                    .memoryBarrierCount = 0,
                    .pMemoryBarriers = nullptr,
                    .bufferMemoryBarrierCount = 1,
                    .pBufferMemoryBarriers = &barrier,
                    .imageMemoryBarrierCount = 0,
                    .pImageMemoryBarriers = nullptr,
                };

                vkCmdPipelineBarrier2(cmdBuf, &dependency);
            }
        }
        else
        {
            std::memcpy(m_MappedData + offset, data, numBytes);
            if (m_UpdateMode == UpdateMode::AlwaysFlush)
                Flush();
        }
    }

    const uint8_t* Buffer::GetMappedData() const noexcept
    { 
        if (m_UpdateMode == UpdateMode::ReadOnly)
            return m_MappedData; 

        COUST_CORE_WARN("Trying to rand access a block of memory without `VMA_ALLOCATION_CREATE_HOST_ACCESS_RANDOM_BIT`");
        return nullptr;
    }

    VkDeviceSize Buffer::GetSize() const noexcept { return m_Size; }
    
    VmaAllocation Buffer::GetAllocation() const noexcept { return m_Allocation; }

    MemoryDomain Buffer::GetMemoryDomain() const noexcept { return m_Domain; }
    
    bool Buffer::IsValid() const noexcept { return m_Handle != VK_NULL_HANDLE && m_Allocation != VK_NULL_HANDLE; }

    bool Buffer::Construct(const Context& ctx, VkBufferUsageFlags bufferFlags, Usage usage, const uint32_t* relatedQueues)
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
        {
            std::unordered_set<uint32_t> allQueues;
            for (uint32_t i = 0; i < COMMAND_QUEUE_COUNT; ++ i)
            {
                allQueues.insert(relatedQueues[i]);
            }
            if (allQueues.size() > 1)
            {
                std::vector<uint32_t> queue(allQueues.begin(), allQueues.end());
                bci.sharingMode = VK_SHARING_MODE_CONCURRENT;
                bci.queueFamilyIndexCount = (uint32_t) queue.size();
                bci.pQueueFamilyIndices = queue.data();
            }
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
            m_Domain = MemoryDomain::HostAndDevice;
        else if (memPropFlags & VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
            m_Domain = MemoryDomain::HostOnly;
        else
            m_Domain = MemoryDomain::DeviceOnly;

        if (memPropFlags & VK_MEMORY_PROPERTY_HOST_COHERENT_BIT)
            m_UpdateMode = UpdateMode::AutoFlush;
        
        if (m_Domain != MemoryDomain::DeviceOnly && allocationFlags & VMA_ALLOCATION_CREATE_MAPPED_BIT)
            m_MappedData = (uint8_t*) ai.pMappedData;

        return true;
    }
    
    void Buffer::SetAlwaysFlush(bool shouldAlwaysFlush) noexcept
    {
        if (m_UpdateMode == UpdateMode::AutoFlush || m_UpdateMode == UpdateMode::ReadOnly)
            return;
        m_UpdateMode = shouldAlwaysFlush ? UpdateMode::AlwaysFlush : UpdateMode::FlushOnDemand;
    }

    void Buffer::Flush() const
    {
        vmaFlushAllocation(m_VMAAllocator, m_Allocation, 0, VK_WHOLE_SIZE);
    }

    Image::Image(const ConstructParam_Create& param)
        : Base(param.ctx, VK_NULL_HANDLE),
          m_Extent{ param.width, param.height, 1 },
          m_Format(param.format),
          m_SampleCount(param.samples),
          m_MipLevelCount(param.mipLevels)
    {
        VkImageCreateInfo CI 
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .flags = param.createFlags,
            // TODO: for now, we hard coded the imageType as `VK_IMAGE_TYPE_2D`
            .imageType = VK_IMAGE_TYPE_2D,
            .format = param.format,
            .extent = m_Extent,
            .mipLevels = param.mipLevels,
            .arrayLayers = 0,
            .samples = param.samples,
            .tiling = param.tiling,
            .usage = param.usageFlags,
            .initialLayout = param.initialLayout
        };
        {
            std::unordered_set<uint32_t> allQueues;
            for (uint32_t i = 0; i < COMMAND_QUEUE_COUNT; ++ i)
            {
                allQueues.insert(param.relatedQueues[i]);
            }
            if (allQueues.size() > 1)
            {
                std::vector<uint32_t> queue(allQueues.begin(), allQueues.end());
                CI.sharingMode = VK_SHARING_MODE_CONCURRENT;
                CI.queueFamilyIndexCount = (uint32_t) queue.size();
                CI.pQueueFamilyIndices = queue.data();
            }
        }

        VmaAllocationCreateInfo AI{};

        // TODO: for now we hard coded the blitable var as true
        GetImageConfig(param.usage, CI, AI, m_ViewType, m_DefaultLayout, true);

        m_PrimarySubRange.aspectMask = IsDepthStencilFormat(param.format) ? 
            VK_IMAGE_ASPECT_DEPTH_BIT : 
            VK_IMAGE_ASPECT_COLOR_BIT;
        m_PrimarySubRange.baseArrayLayer = 0;
        m_PrimarySubRange.layerCount = CI.arrayLayers;
        m_PrimarySubRange.baseMipLevel = 0;
        m_PrimarySubRange.levelCount = CI.mipLevels;

        bool success = false;
        VmaAllocationInfo info{};
        VK_REPORT(vmaCreateImage(m_Ctx.VmaAlloc, &CI, &AI, &m_Handle, &m_Allocation, &info), success);

        // create image view for the primary subresource range
        GetView(m_PrimarySubRange);

        // the layout transition for texture is deferred until upload
        if (param.usage == Usage::ColorAttachment || param.usage == Usage::DepthStencilAttachment ||
            param.usage == Usage::InputAttachment)
            TransitionLayout(m_Ctx.CmdBufCacheGraphics->Get(), m_DefaultLayout, m_PrimarySubRange);

        if (success)
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, ToString(param.usage));
            else  
                COUST_CORE_WARN("Image created without a debug name");
        }
        else  
            m_Handle = VK_NULL_HANDLE;
    }

    Image::Image(const ConstructParam_Wrap& param)
        : Base(param.ctx, param.handle),
          m_Extent{param.width, param.height, 1},
          m_Format(param.format),
          m_SampleCount(param.samples),
          // we won't sample this image, so this is just a dummy value
          m_MipLevelCount(0),
          m_ViewType(VK_IMAGE_VIEW_TYPE_2D)
    {

        if (param.dedicatedName)
            SetDedicatedDebugName(param.dedicatedName);
        else if (param.scopeName)
            SetDefaultDebugName(param.scopeName, nullptr);
    }

    Image::Image(Image&& other) noexcept
        : Base(std::forward<Base>(other)),
          m_CachedImageView(std::move(other.m_CachedImageView)),
          m_SubRangeLayouts(std::move(other.m_SubRangeLayouts)),
          m_MSAAImage(std::move(other.m_MSAAImage)),
          m_Extent(other.m_Extent),
          m_PrimarySubRange(other.m_PrimarySubRange),
          m_Format(other.m_Format),
          m_Allocation(other.m_Allocation),
          m_ViewType(other.m_ViewType),
          m_SampleCount(other.m_SampleCount),
          m_MipLevelCount(other.m_MipLevelCount)
    {
    }

    Image::~Image()
    {
        // if we perform the actual allocation, then it's our responsiblity to destroy it
        if (m_Allocation != VK_NULL_HANDLE)
            vmaDestroyImage(m_Ctx.VmaAlloc, m_Handle, m_Allocation);
    }

    void Image::Update(StagePool& stagePool, const UpdateParam& p)
    {
        VkFormat linearFormat = UnpackSRGBFormat(m_Format);
        size_t dataSize = p.width * p.height * GetBytePerPixelFromFormat(p.dataFormat);
        VkCommandBuffer cmdBuf = m_Ctx.CmdBufCacheGraphics->Get();
        VkImageAspectFlags aspect = IsDepthStencilFormat(m_Format) ? 
            VK_IMAGE_ASPECT_DEPTH_BIT : 
            VK_IMAGE_ASPECT_COLOR_BIT;

        const bool needFormatConversion = p.dataFormat != VK_FORMAT_UNDEFINED && p.dataFormat != linearFormat;
        const bool needResizing = p.width != m_Extent.width || p.height != m_Extent.height;
        // If format conversion or resizing is needed, we use blit
        if (needFormatConversion || needResizing)
        {
            auto stagingImg = stagePool.AcquireStagingImage(p.dataFormat, p.width, p.height);
            stagingImg->Update(p.data, dataSize);

            // we always copy from entrie region to entire region
            VkOffset3D srcRect[2] = { { 0, 0, 0 }, { int32_t(p.width), int32_t(p.height), 1 }};
            VkOffset3D dstRect[2] = { { 0, 0, 0 }, { int32_t(m_Extent.width), int32_t(m_Extent.height), 1}};
            VkImageBlit blitInfo 
            {
                .srcSubresource = 
                {
                    .aspectMask = aspect,
                    .mipLevel = 0,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
                .srcOffsets = { srcRect[0], srcRect[1] },
                .dstSubresource = 
                {
                    .aspectMask = aspect,
                    .mipLevel = p.dstImageMipmapLevel,
                    .baseArrayLayer = p.dstImageLayer,
                    .layerCount = p.dstImageLayerCount,
                },
                .dstOffsets= { dstRect[0], dstRect[1] },
            };

            VkImageSubresourceRange range
            {
                .aspectMask = aspect,
                .baseMipLevel = p.dstImageMipmapLevel,
                .levelCount = 1,
                .baseArrayLayer = p.dstImageLayer,
                .layerCount = p.dstImageLayerCount,
            };

            TransitionLayout(cmdBuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
            vkCmdBlitImage(cmdBuf, stagingImg->GetHandle(), VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, 
                m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blitInfo, VK_FILTER_NEAREST);
            TransitionLayout(cmdBuf, m_DefaultLayout, range);
        }
        // Or we can directly copy it from buffer
        else  
        {
            auto stagingBuf = stagePool.AcquireStagingBuffer(dataSize);
            stagingBuf->Update(stagePool, p.data, dataSize);

            VkBufferImageCopy copyInfo 
            {
                .bufferOffset = 0,
                .bufferRowLength = 0,
                .bufferImageHeight = 0,
                .imageSubresource = 
                {
                    .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                    .mipLevel = p.dstImageMipmapLevel,
                    .baseArrayLayer = p.dstImageLayer,
                    .layerCount = p.dstImageLayerCount,
                },
                .imageOffset = { 0, 0, 0 },
                .imageExtent = { p.width, p.height, 1 },
            };

            VkImageSubresourceRange range
            {
                .aspectMask = aspect,
                .baseMipLevel = p.dstImageMipmapLevel,
                .levelCount = 1,
                .baseArrayLayer = p.dstImageLayer,
                .layerCount = p.dstImageLayerCount,
            };

            TransitionLayout(cmdBuf, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, range);
            vkCmdCopyBufferToImage(cmdBuf, stagingBuf->GetHandle(), m_Handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyInfo);
            TransitionLayout(cmdBuf, m_DefaultLayout, range);
        }
    }

    void Image::TransitionLayout(VkCommandBuffer cmdBuf, VkImageLayout newLayout, VkImageSubresourceRange subRange)
    {
        const uint32_t layerFirst = subRange.baseArrayLayer;
        const uint32_t layerLast = subRange.baseArrayLayer + subRange.layerCount - 1;
        const uint32_t levelFirst = subRange.baseMipLevel;
        const uint32_t levelLast = subRange.baseMipLevel + subRange.levelCount - 1;
        const VkImageLayout oldLayout = GetLayout(layerFirst, levelFirst);

        // Check if the layout in the `subRange` is consistent
        if (oldLayout != VK_IMAGE_LAYOUT_UNDEFINED) 
        {
            for (uint32_t layer = layerFirst; layer <= layerLast; ++ layer)
            {
                for (uint32_t level = levelFirst; level <= levelLast; ++ level)
                {
                    if (GetLayout(layer, level) != oldLayout)
                    {
                        COUST_CORE_ERROR("Try to transition image subresource with inconsisitent image layouts");
                        return;
                    }
                }
            }
        }

        TransitionImageLayout(cmdBuf, ImageBlitTransition( 
            VkImageMemoryBarrier2
            {
                .oldLayout = oldLayout,
                .newLayout = newLayout,
                .image = m_Handle,
                .subresourceRange = subRange,
            }));

        // clear the old layout for undefined new layout
        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED)
        {
            for (uint32_t layer = layerFirst; layer <= layerLast; ++layer)
            {
                auto iterFirst = m_SubRangeLayouts.lower_bound((layer << 16) | levelFirst);
                auto iterLast = m_SubRangeLayouts.upper_bound((layer << 16) | levelLast);
                m_SubRangeLayouts.erase(iterFirst, iterLast);
            }
        }
        // change the old layout to the new layout
        else 
        {
            for (uint32_t layer = layerFirst; layer <= layerLast; ++layer)
            {
                for (uint32_t level = levelFirst; level <= levelLast; ++level)
                {
                    m_SubRangeLayouts[(layer << 16) | level] = newLayout;
                }
            }
        }
    }

    VkImageLayout Image::GetLayout(uint32_t layer, uint32_t level) const noexcept
    {
        const uint32_t key = (layer << 16) | level;
        auto iter = m_SubRangeLayouts.find(key);
        if (iter != m_SubRangeLayouts.end())
            return iter->second;
        else
            return VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void Image::ChangeLayout(uint32_t layer, uint32_t level, VkImageLayout newLayout)
    {
        const uint32_t key = (layer << 16) | level;
        if (newLayout == VK_IMAGE_LAYOUT_UNDEFINED) 
        {
            if (auto iter = m_SubRangeLayouts.find(key); iter != m_SubRangeLayouts.end())
                m_SubRangeLayouts.erase(iter);
        }
        else 
        {
            m_SubRangeLayouts[key] = newLayout;
        }
    }

    const Image::View* Image::GetView(VkImageSubresourceRange subRange)
    {
        if (auto iter = m_CachedImageView.find(subRange); iter != m_CachedImageView.end())
        {
            return &iter->second;
        }

        View::ConstructParam p 
        {
            .ctx = m_Ctx,
            .image = *this,
            .type = m_ViewType,
            .subRange = subRange,
            .scopeName = m_DebugName.c_str(),
        };
        View v { p };
        if (!View::CheckValidation(v))
            return nullptr;
        
        m_CachedImageView.emplace(subRange, std::move(v));
        return &m_CachedImageView.at(subRange);
    }

    const Image::View* Image::GetSingleLayerView(VkImageAspectFlags aspect, uint32_t layer, uint32_t mipLevel)
    {
        return GetView(VkImageSubresourceRange{
            .aspectMask = aspect,
            .baseMipLevel = mipLevel,
            .levelCount = 1,
            .baseArrayLayer = layer,
            .layerCount = 1,
        });
    }

    VkImageLayout Image::GetPrimaryLayout() const noexcept
    {
        return GetLayout(m_PrimarySubRange.baseArrayLayer, m_PrimarySubRange.baseMipLevel);
    }

    const Image::View* Image::GetPrimaryView() const
    {
        return &m_CachedImageView.at(m_PrimarySubRange);
    }

    VkImageSubresourceRange Image::GetPrimarySubRange() const noexcept { return m_PrimarySubRange; }

    void Image::SetPrimarySubRange(uint32_t minMipmapLevel, uint32_t maxMipmaplevel)
    {
        maxMipmaplevel = std::min(maxMipmaplevel, m_MipLevelCount - 1);
        m_PrimarySubRange.baseMipLevel = minMipmapLevel;
        m_PrimarySubRange.levelCount = maxMipmaplevel - minMipmapLevel + 1;
        GetView(m_PrimarySubRange);
    }

    VkExtent3D Image::GetExtent() const noexcept { return m_Extent; }

    VkFormat Image::GetFormat() const noexcept { return m_Format; }

    VmaAllocation Image::GetAllocation() const noexcept { return m_Allocation; }

    VkSampleCountFlagBits Image::GetSampleCount() const noexcept { return m_SampleCount; }

    std::shared_ptr<Image> Image::GetMSAAImage() const noexcept { return m_MSAAImage; }

    void Image::SetMASSImage(std::shared_ptr<Image> msaaImage) noexcept { m_MSAAImage = msaaImage; } 

    Image::View::View(const ConstructParam& param) noexcept
        : Base(param.ctx, VK_NULL_HANDLE), 
          m_Image(param.image)
    {
        if (param.subRange.layerCount == 0 || param.subRange.levelCount == 0)
        {
            COUST_CORE_ERROR("Can't create image view with 0 mipmap level count or 0 array layer count");
            return;
        }
            
        VkImageViewCreateInfo ci
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
            .flags = 0,
            .image = m_Image.GetHandle(),
            .viewType = param.type,
            .format = m_Image.GetFormat(),
            .components = 
            {
                .r = VK_COMPONENT_SWIZZLE_IDENTITY,
                .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                .a = VK_COMPONENT_SWIZZLE_IDENTITY,
            },
            .subresourceRange = param.subRange,
        };

        bool success = false;
        VK_REPORT(vkCreateImageView(m_Ctx.Device, &ci, nullptr, &m_Handle), success);

        if (success)
        {
            if (param.dedicatedName)
                SetDedicatedDebugName(param.dedicatedName);
            else if (param.scopeName)
                SetDefaultDebugName(param.scopeName, param.image.m_DebugName.c_str());
        }
        else
            m_Handle = VK_NULL_HANDLE;
    }

    Image::View::View(View&& other) noexcept
        : Base(std::forward<Base>(other)),
          m_Image(other.m_Image)
    {
    }
    
    Image::View::~View()
    {
        if (m_Handle != VK_NULL_HANDLE)
            vkDestroyImageView(m_Ctx.Device, m_Handle, nullptr);
    }

    const Image& Image::View::GetImage() const { return m_Image; }

    HostImage::HostImage(const ConstructParam& param)
        : Base(param.ctx, VK_NULL_HANDLE), m_Format(param.format)
    {
        VkImageCreateInfo CI
        {
            .sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
            .imageType = VK_IMAGE_TYPE_2D,
            .format = param.format,
            .extent = 
            {
                .width = param.width,
                .height = param.height,
                .depth = 1,
            },
            .mipLevels = 1,
            .arrayLayers = 1,
            .samples = VK_SAMPLE_COUNT_1_BIT,
            .tiling = VK_IMAGE_TILING_LINEAR,
            .usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT,
            .initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
        };

        VmaAllocationCreateInfo AI 
        {
            .flags = VMA_ALLOCATION_CREATE_MAPPED_BIT | VMA_ALLOCATION_CREATE_HOST_ACCESS_SEQUENTIAL_WRITE_BIT,
            .usage = VMA_MEMORY_USAGE_AUTO_PREFER_DEVICE,
        };

        bool success = false;
        VmaAllocationInfo allocInfo{};
        VK_REPORT(vmaCreateImage(m_Ctx.VmaAlloc, &CI, &AI, &m_Handle, &m_Allocation, &allocInfo), success);

        m_MappedData = allocInfo.pMappedData;

        if (!success)
        {
            m_Handle = VK_NULL_HANDLE;
            m_Allocation = VK_NULL_HANDLE;
            return;
        }

        auto aspect = GetAspect();

        TransitionImageLayout(m_Ctx.CmdBufCacheGraphics->Get(), ImageBlitTransition(
            VkImageMemoryBarrier2
            {
                .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
                .newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                .image = m_Handle,
                .subresourceRange = 
                {
                    .aspectMask = aspect,
                    .baseMipLevel = 0,
                    .levelCount = 1,
                    .baseArrayLayer = 0,
                    .layerCount = 1,
                },
            }));

        SetDefaultDebugName("StagePool", nullptr);
    }


    HostImage::~HostImage()
    {
        if (m_Handle != VK_NULL_HANDLE && m_Allocation != VK_NULL_HANDLE)
        {
            vmaDestroyImage(m_Ctx.VmaAlloc, m_Handle, m_Allocation);
        }
    }

    void HostImage::Update(const void* data, size_t size)
    {
        memcpy(m_MappedData, data, size);
        // auto flush
        vmaFlushAllocation(m_Ctx.VmaAlloc, m_Allocation, 0, size);
    }

    VkImageAspectFlags HostImage::GetAspect() const noexcept
    {
        return IsDepthStencilFormat(m_Format) ? 
            VK_IMAGE_ASPECT_DEPTH_BIT : 
            VK_IMAGE_ASPECT_COLOR_BIT;
    }

    VkFormat HostImage::GetFormat() const noexcept { return m_Format; }

    uint32_t HostImage::GetWidth() const noexcept { return m_Width; }

    uint32_t HostImage::GetHeight() const noexcept { return m_Height; }
}