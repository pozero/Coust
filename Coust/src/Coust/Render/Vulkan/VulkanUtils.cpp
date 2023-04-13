#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Utils/Hash.h"

namespace Coust::Render::VK 
{
    VkFormat GetNomalizedFormat(VkFormat format) noexcept
    {
        switch (format)
        {
            case VK_FORMAT_R8_UINT: 			    return VK_FORMAT_R8_UNORM;
            case VK_FORMAT_R8_SINT: 			    return VK_FORMAT_R8_SNORM;
            case VK_FORMAT_R8G8_UINT: 			    return VK_FORMAT_R8G8_UNORM;
            case VK_FORMAT_R8G8_SINT: 			    return VK_FORMAT_R8G8_SNORM;
            case VK_FORMAT_R8G8B8_UINT: 		    return VK_FORMAT_R8G8B8_UNORM;
            case VK_FORMAT_R8G8B8_SINT: 		    return VK_FORMAT_R8G8B8_SNORM;
            case VK_FORMAT_R8G8B8A8_UINT: 		    return VK_FORMAT_R8G8B8A8_UNORM;
            case VK_FORMAT_R8G8B8A8_SINT: 		    return VK_FORMAT_R8G8B8A8_SNORM;
            case VK_FORMAT_B8G8R8_UINT: 		    return VK_FORMAT_B8G8R8_UNORM;
            case VK_FORMAT_B8G8R8_SINT: 		    return VK_FORMAT_B8G8R8_SNORM;
            case VK_FORMAT_B8G8R8A8_UINT: 		    return VK_FORMAT_B8G8R8A8_UNORM;
            case VK_FORMAT_B8G8R8A8_SINT: 			return VK_FORMAT_B8G8R8A8_SNORM;

            case VK_FORMAT_A8B8G8R8_UINT_PACK32: 	return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
            case VK_FORMAT_A8B8G8R8_SINT_PACK32: 	return VK_FORMAT_A8B8G8R8_SNORM_PACK32;

            case VK_FORMAT_A2R10G10B10_UINT_PACK32: return VK_FORMAT_A2R10G10B10_UNORM_PACK32;
            case VK_FORMAT_A2R10G10B10_SINT_PACK32: return VK_FORMAT_A2R10G10B10_SNORM_PACK32;
            case VK_FORMAT_A2B10G10R10_UINT_PACK32: return VK_FORMAT_A2B10G10R10_UNORM_PACK32;
            case VK_FORMAT_A2B10G10R10_SINT_PACK32: return VK_FORMAT_A2B10G10R10_SNORM_PACK32;

            case VK_FORMAT_R16_SINT:                return VK_FORMAT_R16_SNORM;
            case VK_FORMAT_R16_UINT:                return VK_FORMAT_R16_UNORM;
            case VK_FORMAT_R16G16_SINT:             return VK_FORMAT_R16G16_SNORM;
            case VK_FORMAT_R16G16_UINT:             return VK_FORMAT_R16G16_UNORM;
            case VK_FORMAT_R16G16B16_SINT:          return VK_FORMAT_R16G16B16_SNORM;
            case VK_FORMAT_R16G16B16_UINT:          return VK_FORMAT_R16G16B16_UNORM;
            case VK_FORMAT_R16G16B16A16_SINT:       return VK_FORMAT_R16G16B16A16_SNORM;
            case VK_FORMAT_R16G16B16A16_UINT:       return VK_FORMAT_R16G16B16A16_UNORM;

            default: 								return format;
        }
    }

    VkFormat UnpackSRGBFormat(VkFormat srgbFormat) noexcept
    {
        switch (srgbFormat)
        {
            case VK_FORMAT_R8_SRGB: 			   	return VK_FORMAT_R8_UNORM;
            case VK_FORMAT_R8G8_SRGB: 				return VK_FORMAT_R8G8_UNORM;
            case VK_FORMAT_R8G8B8_SRGB: 			return VK_FORMAT_R8G8B8_UNORM;
            case VK_FORMAT_B8G8R8_SRGB: 			return VK_FORMAT_B8G8R8_UNORM;
            case VK_FORMAT_R8G8B8A8_SRGB: 			return VK_FORMAT_R8G8B8A8_UNORM;
            case VK_FORMAT_B8G8R8A8_SRGB: 			return VK_FORMAT_B8G8R8A8_UNORM;
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32: 	return VK_FORMAT_A8B8G8R8_UNORM_PACK32;
            default: 								return srgbFormat;
        }
    }

    VkImageMemoryBarrier2 ImageBlitTransition(VkImageMemoryBarrier2 barrier) noexcept
    {
        switch (barrier.newLayout)
        {
            // copy dst
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
            case VK_IMAGE_LAYOUT_GENERAL:
                barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
                barrier.dstAccessMask = VK_ACCESS_2_SHADER_READ_BIT;
                // all the copy, blit, resolve and clear operation
                barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                // synchronize before fragment shader stage
                barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
                break;

            // attachment
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
            default:
                barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_READ_BIT;
                barrier.dstAccessMask = VK_ACCESS_2_NONE;
                barrier.srcStageMask = VK_PIPELINE_STAGE_2_TRANSFER_BIT;
                // synchronize before all the stage
                barrier.dstStageMask = VK_PIPELINE_STAGE_2_TOP_OF_PIPE_BIT;
                break;
        }

        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

        return barrier;
    }

    void TransitionImageLayout(VkCommandBuffer cmdBuf, VkImageMemoryBarrier2 barrier) noexcept
    {
        // https://github.com/KhronosGroup/Vulkan-Guide/blob/main/chapters/extensions/VK_KHR_synchronization2.adoc
        // Additionally, with VK_KHR_synchronization2, if oldLayout is equal to newLayout, no layout transition is performed and the image contents are preserved. 

        VkDependencyInfo dependency
        {
            .sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO,
            // .dependencyFlags;
            .memoryBarrierCount = 0,
            .pMemoryBarriers = nullptr,
            .bufferMemoryBarrierCount = 0,
            .pBufferMemoryBarriers = nullptr,
            .imageMemoryBarrierCount = 1,
            .pImageMemoryBarriers = &barrier,
        };
        vkCmdPipelineBarrier2(cmdBuf, &dependency);
    }

    uint32_t GetBytePerPixelFromFormat(VkFormat format) noexcept
    {
        // Grab from spec
        switch (format)
        {
            case VK_FORMAT_UNDEFINED:
                return 0;

            case VK_FORMAT_R4G4_UNORM_PACK8:
            case VK_FORMAT_R8_UNORM:
            case VK_FORMAT_R8_SNORM:
            case VK_FORMAT_R8_USCALED:
            case VK_FORMAT_R8_SSCALED:
            case VK_FORMAT_R8_UINT:
            case VK_FORMAT_R8_SINT:
            case VK_FORMAT_R8_SRGB:
            case VK_FORMAT_S8_UINT:
                return 1;

            case VK_FORMAT_R10X6_UNORM_PACK16:
            case VK_FORMAT_R12X4_UNORM_PACK16:
            case VK_FORMAT_A4R4G4B4_UNORM_PACK16:
            case VK_FORMAT_A4B4G4R4_UNORM_PACK16:
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16:
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16:
            case VK_FORMAT_R5G6B5_UNORM_PACK16:
            case VK_FORMAT_B5G6R5_UNORM_PACK16:
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16:
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16:
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16:
            case VK_FORMAT_R8G8_UNORM:
            case VK_FORMAT_R8G8_SNORM:
            case VK_FORMAT_R8G8_USCALED:
            case VK_FORMAT_R8G8_SSCALED:
            case VK_FORMAT_R8G8_UINT:
            case VK_FORMAT_R8G8_SINT:
            case VK_FORMAT_R8G8_SRGB:
            case VK_FORMAT_R16_UNORM:
            case VK_FORMAT_R16_SNORM:
            case VK_FORMAT_R16_USCALED:
            case VK_FORMAT_R16_SSCALED:
            case VK_FORMAT_R16_UINT:
            case VK_FORMAT_R16_SINT:
            case VK_FORMAT_R16_SFLOAT:
            case VK_FORMAT_D16_UNORM:
                return 2;

            case VK_FORMAT_R8G8B8_UNORM:
            case VK_FORMAT_R8G8B8_SNORM:
            case VK_FORMAT_R8G8B8_USCALED:
            case VK_FORMAT_R8G8B8_SSCALED:
            case VK_FORMAT_R8G8B8_UINT:
            case VK_FORMAT_R8G8B8_SINT:
            case VK_FORMAT_R8G8B8_SRGB:
            case VK_FORMAT_B8G8R8_UNORM:
            case VK_FORMAT_B8G8R8_SNORM:
            case VK_FORMAT_B8G8R8_USCALED:
            case VK_FORMAT_B8G8R8_SSCALED:
            case VK_FORMAT_B8G8R8_UINT:
            case VK_FORMAT_B8G8R8_SINT:
            case VK_FORMAT_B8G8R8_SRGB:
            case VK_FORMAT_D16_UNORM_S8_UINT:
                return 3;

            case VK_FORMAT_R10X6G10X6_UNORM_2PACK16:
            case VK_FORMAT_R12X4G12X4_UNORM_2PACK16:
            case VK_FORMAT_R8G8B8A8_UNORM:
            case VK_FORMAT_R8G8B8A8_SNORM:
            case VK_FORMAT_R8G8B8A8_USCALED:
            case VK_FORMAT_R8G8B8A8_SSCALED:
            case VK_FORMAT_R8G8B8A8_UINT:
            case VK_FORMAT_R8G8B8A8_SINT:
            case VK_FORMAT_R8G8B8A8_SRGB:
            case VK_FORMAT_B8G8R8A8_UNORM:
            case VK_FORMAT_B8G8R8A8_SNORM:
            case VK_FORMAT_B8G8R8A8_USCALED:
            case VK_FORMAT_B8G8R8A8_SSCALED:
            case VK_FORMAT_B8G8R8A8_UINT:
            case VK_FORMAT_B8G8R8A8_SINT:
            case VK_FORMAT_B8G8R8A8_SRGB:
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32:
            case VK_FORMAT_A8B8G8R8_SNORM_PACK32:
            case VK_FORMAT_A8B8G8R8_USCALED_PACK32:
            case VK_FORMAT_A8B8G8R8_SSCALED_PACK32:
            case VK_FORMAT_A8B8G8R8_UINT_PACK32:
            case VK_FORMAT_A8B8G8R8_SINT_PACK32:
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32:
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32:
            case VK_FORMAT_A2R10G10B10_SNORM_PACK32:
            case VK_FORMAT_A2R10G10B10_USCALED_PACK32:
            case VK_FORMAT_A2R10G10B10_SSCALED_PACK32:
            case VK_FORMAT_A2R10G10B10_UINT_PACK32:
            case VK_FORMAT_A2R10G10B10_SINT_PACK32:
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32:
            case VK_FORMAT_A2B10G10R10_SNORM_PACK32:
            case VK_FORMAT_A2B10G10R10_USCALED_PACK32:
            case VK_FORMAT_A2B10G10R10_SSCALED_PACK32:
            case VK_FORMAT_A2B10G10R10_UINT_PACK32:
            case VK_FORMAT_A2B10G10R10_SINT_PACK32:
            case VK_FORMAT_R16G16_UNORM:
            case VK_FORMAT_R16G16_SNORM:
            case VK_FORMAT_R16G16_USCALED:
            case VK_FORMAT_R16G16_SSCALED:
            case VK_FORMAT_R16G16_UINT:
            case VK_FORMAT_R16G16_SINT:
            case VK_FORMAT_R16G16_SFLOAT:
            case VK_FORMAT_R32_UINT:
            case VK_FORMAT_R32_SINT:
            case VK_FORMAT_R32_SFLOAT:
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32:
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32:
            case VK_FORMAT_X8_D24_UNORM_PACK32:
            case VK_FORMAT_D32_SFLOAT:
            case VK_FORMAT_D24_UNORM_S8_UINT:
                return 4;
            
            case VK_FORMAT_R16G16B16_UNORM:
            case VK_FORMAT_R16G16B16_SNORM:
            case VK_FORMAT_R16G16B16_USCALED:
            case VK_FORMAT_R16G16B16_SSCALED:
            case VK_FORMAT_R16G16B16_UINT:
            case VK_FORMAT_R16G16B16_SINT:
            case VK_FORMAT_R16G16B16_SFLOAT:
                return 6;

            case VK_FORMAT_R16G16B16A16_UNORM:
            case VK_FORMAT_R16G16B16A16_SNORM:
            case VK_FORMAT_R16G16B16A16_USCALED:
            case VK_FORMAT_R16G16B16A16_SSCALED:
            case VK_FORMAT_R16G16B16A16_UINT:
            case VK_FORMAT_R16G16B16A16_SINT:
            case VK_FORMAT_R16G16B16A16_SFLOAT:
            case VK_FORMAT_R32G32_UINT:
            case VK_FORMAT_R32G32_SINT:
            case VK_FORMAT_R32G32_SFLOAT:
            case VK_FORMAT_R64_UINT:
            case VK_FORMAT_R64_SINT:
            case VK_FORMAT_R64_SFLOAT:
            // The spec says:
            // VK_FORMAT_D32_SFLOAT_S8_UINT specifies a two-component format that has 32 signed float bits in
            // the depth component and 8 unsigned integer bits in the stencil component. There are
            // **OPTIONALLY** 24 bits that are unused.
            case VK_FORMAT_D32_SFLOAT_S8_UINT:
                return 8;

            default:
                COUST_CORE_WARN("VkFormat no.{} not added to `GetBytePerPixelFromFormat` yet, return 0", (int) format);
                return 0;
        }
    }

    bool IsDepthOnlyFormat(VkFormat format) noexcept
    {
        return format == VK_FORMAT_D32_SFLOAT || 
               format == VK_FORMAT_D16_UNORM;
    }

    bool IsDepthStencilFormat(VkFormat format) noexcept
    {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
                format == VK_FORMAT_D24_UNORM_S8_UINT ||
               format == VK_FORMAT_D16_UNORM_S8_UINT ||
               IsDepthOnlyFormat(format);
    }

    const char* ToString(VkResult result) noexcept
    {
        switch (result)
        {
        case			0:		return "VK_SUCCESS";
        case			1:		return "VK_NOT_READY";
        case			2:		return "VK_TIMEOUT";
        case			3:		return "VK_EVENT_SET";
        case			4:		return "VK_EVENT_RESET";
        case			5:		return "VK_INCOMPLETE";
        case		   -1:		return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case		   -2:		return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case		   -3:		return "VK_ERROR_INITIALIZATION_FAILED";
        case		   -4:		return "VK_ERROR_DEVICE_LOST";
        case		   -5:		return "VK_ERROR_MEMORY_MAP_FAILED";
        case		   -6:		return "VK_ERROR_LAYER_NOT_PRESENT";
        case		   -7:		return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case		   -8:		return "VK_ERROR_FEATURE_NOT_PRESENT";
        case		   -9:		return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case		  -10:		return "VK_ERROR_TOO_MANY_OBJECTS";
        case		  -11:		return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case		  -12:		return "VK_ERROR_FRAGMENTED_POOL";
        case		  -13:		return "VK_ERROR_UNKNOWN";
        case  -1000069000:		return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case  -1000072003:		return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case  -1000161000:		return "VK_ERROR_FRAGMENTATION";
        case  -1000257000:		return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case   1000297000:		return "VK_PIPELINE_COMPILE_REQUIRED";
        case  -1000000000:		return "VK_ERROR_SURFACE_LOST_KHR";
        case  -1000000001:		return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case   1000001003:		return "VK_SUBOPTIMAL_KHR";
        case  -1000001004:		return "VK_ERROR_OUT_OF_DATE_KHR";
        case  -1000003001:		return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case  -1000011001:		return "VK_ERROR_VALIDATION_FAILED_EXT";
        case  -1000012000:		return "VK_ERROR_INVALID_SHADER_NV";
        case  -1000023000:		return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case  -1000023001:		return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case  -1000023002:		return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case  -1000023003:		return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case  -1000023004:		return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case  -1000023005:		return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case  -1000158000:		return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case  -1000174001:		return "VK_ERROR_NOT_PERMITTED_KHR";
        case  -1000255000:		return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case   1000268000:		return "VK_THREAD_IDLE_KHR";
        case   1000268001:		return "VK_THREAD_DONE_KHR";
        case   1000268002:		return "VK_OPERATION_DEFERRED_KHR";
        case   1000268003:		return "VK_OPERATION_NOT_DEFERRED_KHR";
        case  -1000338000:		return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        default:				return "Unknown vulkan error code";
        }
    }
    
    const char* ToString(VkObjectType objType) noexcept
    {
        switch (objType)
        {
            case VK_OBJECT_TYPE_UNKNOWN: 							return "UNKNOWN";
            case VK_OBJECT_TYPE_INSTANCE: 							return "INSTANCE";
            case VK_OBJECT_TYPE_PHYSICAL_DEVICE: 					return "PHYSICAL_DEVICE";
            case VK_OBJECT_TYPE_DEVICE: 							return "DEVICE";
            case VK_OBJECT_TYPE_QUEUE: 								return "QUEUE";
            case VK_OBJECT_TYPE_SEMAPHORE: 							return "SEMAPHORE";
            case VK_OBJECT_TYPE_COMMAND_BUFFER: 					return "COMMAND_BUFFER";
            case VK_OBJECT_TYPE_FENCE: 								return "FENCE";
            case VK_OBJECT_TYPE_DEVICE_MEMORY: 						return "DEVICE_MEMORY";
            case VK_OBJECT_TYPE_BUFFER: 							return "BUFFER";
            case VK_OBJECT_TYPE_IMAGE: 								return "IMAGE";
            case VK_OBJECT_TYPE_EVENT: 								return "EVENT";
            case VK_OBJECT_TYPE_QUERY_POOL: 						return "QUERY_POOL";
            case VK_OBJECT_TYPE_BUFFER_VIEW: 						return "BUFFER_VIEW";
            case VK_OBJECT_TYPE_IMAGE_VIEW: 						return "IMAGE_VIEW";
            case VK_OBJECT_TYPE_SHADER_MODULE: 						return "SHADER_MODULE";
            case VK_OBJECT_TYPE_PIPELINE_CACHE: 					return "PIPELINE_CACHE";
            case VK_OBJECT_TYPE_PIPELINE_LAYOUT: 					return "PIPELINE_LAYOUT";
            case VK_OBJECT_TYPE_RENDER_PASS: 						return "RENDER_PASS";
            case VK_OBJECT_TYPE_PIPELINE: 							return "PIPELINE";
            case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT: 				return "DESCRIPTOR_SET_LAYOUT";
            case VK_OBJECT_TYPE_SAMPLER: 							return "SAMPLER";
            case VK_OBJECT_TYPE_DESCRIPTOR_POOL: 					return "DESCRIPTOR_POOL";
            case VK_OBJECT_TYPE_DESCRIPTOR_SET: 					return "DESCRIPTOR_SET";
            case VK_OBJECT_TYPE_FRAMEBUFFER: 						return "FRAMEBUFFER";
            case VK_OBJECT_TYPE_COMMAND_POOL: 						return "COMMAND_POOL";
            case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION: 			return "SAMPLER_YCBCR_CONVERSION";
            case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE: 		return "DESCRIPTOR_UPDATE_TEMPLATE";
            case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT: 					return "PRIVATE_DATA_SLOT";
            case VK_OBJECT_TYPE_SURFACE_KHR: 						return "SURFACE_KHR";
            case VK_OBJECT_TYPE_SWAPCHAIN_KHR: 						return "SWAPCHAIN_KHR";
            case VK_OBJECT_TYPE_DISPLAY_KHR: 						return "DISPLAY_KHR";
            case VK_OBJECT_TYPE_DISPLAY_MODE_KHR: 					return "DISPLAY_MODE_KHR";
            case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT: 			return "DEBUG_REPORT_CALLBACK_EXT";
            case VK_OBJECT_TYPE_VIDEO_SESSION_KHR: 					return "VIDEO_SESSION_KHR";
            case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR: 		return "VIDEO_SESSION_PARAMETERS_KHR";
            case VK_OBJECT_TYPE_CU_MODULE_NVX: 						return "CU_MODULE_NVX";
            case VK_OBJECT_TYPE_CU_FUNCTION_NVX: 					return "CU_FUNCTION_NVX";
            case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT: 			return "DEBUG_UTILS_MESSENGER_EXT";
            case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR: 		return "ACCELERATION_STRUCTURE_KHR";
            case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT: 				return "VALIDATION_CACHE_EXT";
            case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV: 			return "ACCELERATION_STRUCTURE_NV";
            case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL: 	return "PERFORMANCE_CONFIGURATION_INTEL";
            case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR: 			return "DEFERRED_OPERATION_KHR";
            case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV: 		return "INDIRECT_COMMANDS_LAYOUT_NV";
            case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA: 			return "BUFFER_COLLECTION_FUCHSIA";
            case VK_OBJECT_TYPE_MICROMAP_EXT: 						return "MICROMAP_EXT";
            case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV: 			return "OPTICAL_FLOW_SESSION_NV";
            default:												return "Unknown Vulkan Object Type";
        }
    }
    
    const char* ToString(VkAccessFlagBits bit) noexcept
    {
        switch (bit) 
        {
            case VK_ACCESS_INDIRECT_COMMAND_READ_BIT:                       return "VK_ACCESS_INDIRECT_COMMAND_READ_BIT";
            case VK_ACCESS_INDEX_READ_BIT:                                  return "VK_ACCESS_INDEX_READ_BIT";
            case VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT:                       return "VK_ACCESS_VERTEX_ATTRIBUTE_READ_BIT";
            case VK_ACCESS_UNIFORM_READ_BIT:                                return "VK_ACCESS_UNIFORM_READ_BIT";
            case VK_ACCESS_INPUT_ATTACHMENT_READ_BIT:                       return "VK_ACCESS_INPUT_ATTACHMENT_READ_BIT";
            case VK_ACCESS_SHADER_READ_BIT:                                 return "VK_ACCESS_SHADER_READ_BIT";
            case VK_ACCESS_SHADER_WRITE_BIT:                                return "VK_ACCESS_SHADER_WRITE_BIT";
            case VK_ACCESS_COLOR_ATTACHMENT_READ_BIT:                       return "VK_ACCESS_COLOR_ATTACHMENT_READ_BIT";
            case VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT:                      return "VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT";
            case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT:               return "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT";
            case VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT:              return "VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT";
            case VK_ACCESS_TRANSFER_READ_BIT:                               return "VK_ACCESS_TRANSFER_READ_BIT";
            case VK_ACCESS_TRANSFER_WRITE_BIT:                              return "VK_ACCESS_TRANSFER_WRITE_BIT";
            case VK_ACCESS_HOST_READ_BIT:                                   return "VK_ACCESS_HOST_READ_BIT";
            case VK_ACCESS_HOST_WRITE_BIT:                                  return "VK_ACCESS_HOST_WRITE_BIT";
            case VK_ACCESS_MEMORY_READ_BIT:                                 return "VK_ACCESS_MEMORY_READ_BIT";
            case VK_ACCESS_MEMORY_WRITE_BIT:                                return "VK_ACCESS_MEMORY_WRITE_BIT";
            case VK_ACCESS_NONE:                                            return "VK_ACCESS_NONE";
            case VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT:                return "VK_ACCESS_TRANSFORM_FEEDBACK_WRITE_BIT_EXT";
            case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT:         return "VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_READ_BIT_EXT";
            case VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT:        return "VK_ACCESS_TRANSFORM_FEEDBACK_COUNTER_WRITE_BIT_EXT";
            case VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT:              return "VK_ACCESS_CONDITIONAL_RENDERING_READ_BIT_EXT";
            case VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT:       return "VK_ACCESS_COLOR_ATTACHMENT_READ_NONCOHERENT_BIT_EXT";
            case VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR:             return "VK_ACCESS_ACCELERATION_STRUCTURE_READ_BIT_KHR";
            case VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR:            return "VK_ACCESS_ACCELERATION_STRUCTURE_WRITE_BIT_KHR";
            case VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT:               return "VK_ACCESS_FRAGMENT_DENSITY_MAP_READ_BIT_EXT";
            case VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR:   return "VK_ACCESS_FRAGMENT_SHADING_RATE_ATTACHMENT_READ_BIT_KHR";
            case VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV:                  return "VK_ACCESS_COMMAND_PREPROCESS_READ_BIT_NV";
            case VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV:                 return "VK_ACCESS_COMMAND_PREPROCESS_WRITE_BIT_NV";
            default:                                                        return "Unknown access flag bit";
        }
    }
    
    const char* ToString(VkShaderStageFlagBits bit) noexcept
    {
        switch (bit)
        {
            case VK_SHADER_STAGE_VERTEX_BIT:                    return "VERTEX";
            case VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT:      return "TESSELLATION_CONTROL";
            case VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT:   return "TESSELLATION_EVALUATION";
            case VK_SHADER_STAGE_GEOMETRY_BIT:                  return "GEOMETRY";
            case VK_SHADER_STAGE_FRAGMENT_BIT:                  return "FRAGMENT";
            case VK_SHADER_STAGE_COMPUTE_BIT:                   return "COMPUTE";
            case VK_SHADER_STAGE_ALL_GRAPHICS:                  return "ALL_GRAPHICS";
            case VK_SHADER_STAGE_RAYGEN_BIT_KHR:                return "RAYGEN_KHR";
            case VK_SHADER_STAGE_ANY_HIT_BIT_KHR:               return "ANY_HIT_KHR";
            case VK_SHADER_STAGE_CLOSEST_HIT_BIT_KHR:           return "CLOSEST_HIT_KHR";
            case VK_SHADER_STAGE_MISS_BIT_KHR:                  return "MISS_KHR";
            case VK_SHADER_STAGE_INTERSECTION_BIT_KHR:          return "INTERSECTION_KHR";
            case VK_SHADER_STAGE_CALLABLE_BIT_KHR:              return "CALLABLE_KHR";
            case VK_SHADER_STAGE_TASK_BIT_EXT:                  return "TAS_EXT";
            case VK_SHADER_STAGE_MESH_BIT_EXT:                  return "MESH_EXT";
            case VK_SHADER_STAGE_SUBPASS_SHADING_BIT_HUAWEI:    return "SUBPASS_SHADING_HUAWEI";
            case VK_SHADER_STAGE_CLUSTER_CULLING_BIT_HUAWEI:    return "CLUSTER_CULLIN_HUAWEI";
            default:                                            return "Unknown shader stage flag bit";
        }
    }
    
    const char* ToString(VkCommandBufferLevel level) noexcept
    {
        switch (level)
        {
            case VK_COMMAND_BUFFER_LEVEL_PRIMARY:		return "PRIMARY";
            case VK_COMMAND_BUFFER_LEVEL_SECONDARY:		return "SECONDARY";
            default: 									return "Unknow command buffer level";
        }
    }
    
    const char* ToString(VkDescriptorType type) noexcept
    {
        switch (type) 
        {
            case VK_DESCRIPTOR_TYPE_SAMPLER: 						return "DESCRIPTOR_TYPE_SAMPLER";
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER: 		return "DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER";
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE: 					return "DESCRIPTOR_TYPE_SAMPLED_IMAGE";
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE: 					return "DESCRIPTOR_TYPE_STORAGE_IMAGE";
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER: 			return "DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER";
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER: 			return "DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER";
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER: 				return "DESCRIPTOR_TYPE_UNIFORM_BUFFER";
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER: 				return "DESCRIPTOR_TYPE_STORAGE_BUFFER";
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC: 		return "DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC";
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC: 		return "DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC";
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT: 				return "DESCRIPTOR_TYPE_INPUT_ATTACHMENT";
            case VK_DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK: 			return "DESCRIPTOR_TYPE_INLINE_UNIFORM_BLOCK";
            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR: 	return "DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_KHR";
            case VK_DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV: 		return "DESCRIPTOR_TYPE_ACCELERATION_STRUCTURE_NV";
            case VK_DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM: 		return "DESCRIPTOR_TYPE_SAMPLE_WEIGHT_IMAGE_QCOM";
            case VK_DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM: 		return "DESCRIPTOR_TYPE_BLOCK_MATCH_IMAGE_QCOM";
            case VK_DESCRIPTOR_TYPE_MUTABLE_EXT: 					return "DESCRIPTOR_TYPE_MUTABLE_EXT";
            default:												return "Unknown descriptor type";
        }
    }
    
    const char* ToString(VkBufferUsageFlagBits bit) noexcept
    {
        switch (bit)
        {
            case VK_BUFFER_USAGE_TRANSFER_SRC_BIT: 											return "VK_BUFFER_USAGE_TRANSFER_SRC_BIT";
            case VK_BUFFER_USAGE_TRANSFER_DST_BIT: 											return "VK_BUFFER_USAGE_TRANSFER_DST_BIT";
            case VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT: 									return "VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT";
            case VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT: 									return "VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT";
            case VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT: 										return "VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT";
            case VK_BUFFER_USAGE_STORAGE_BUFFER_BIT: 										return "VK_BUFFER_USAGE_STORAGE_BUFFER_BIT";
            case VK_BUFFER_USAGE_INDEX_BUFFER_BIT: 											return "VK_BUFFER_USAGE_INDEX_BUFFER_BIT";
            case VK_BUFFER_USAGE_VERTEX_BUFFER_BIT: 										return "VK_BUFFER_USAGE_VERTEX_BUFFER_BIT";
            case VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT: 										return "VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT";
            case VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT: 								return "VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT";
            case VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR: 									return "VK_BUFFER_USAGE_VIDEO_DECODE_SRC_BIT_KHR";
            case VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR: 									return "VK_BUFFER_USAGE_VIDEO_DECODE_DST_BIT_KHR";
            case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT: 						return "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_BUFFER_BIT_EXT";
            case VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT: 				return "VK_BUFFER_USAGE_TRANSFORM_FEEDBACK_COUNTER_BUFFER_BIT_EXT";
            case VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT: 							return "VK_BUFFER_USAGE_CONDITIONAL_RENDERING_BIT_EXT";
            case VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR: 		return "VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_BUILD_INPUT_READ_ONLY_BIT_KHR";
            case VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR: 					return "VK_BUFFER_USAGE_ACCELERATION_STRUCTURE_STORAGE_BIT_KHR";
            case VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR: 								return "VK_BUFFER_USAGE_SHADER_BINDING_TABLE_BIT_KHR";
            case VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT: 						return "VK_BUFFER_USAGE_SAMPLER_DESCRIPTOR_BUFFER_BIT_EXT";
            case VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT: 						return "VK_BUFFER_USAGE_RESOURCE_DESCRIPTOR_BUFFER_BIT_EXT";
            case VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT: 				return "VK_BUFFER_USAGE_PUSH_DESCRIPTORS_DESCRIPTOR_BUFFER_BIT_EXT";
            case VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT: 					return "VK_BUFFER_USAGE_MICROMAP_BUILD_INPUT_READ_ONLY_BIT_EXT";
            case VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT: 									return "VK_BUFFER_USAGE_MICROMAP_STORAGE_BIT_EXT";
            default: 																		return "Unknown buffer usage";
        }
    }

    const char* ToString(VkImageUsageFlagBits bit) noexcept
    {
        switch (bit)
        {
            case VK_IMAGE_USAGE_TRANSFER_SRC_BIT:										return "VK_IMAGE_USAGE_TRANSFER_SRC_BIT";
            case VK_IMAGE_USAGE_TRANSFER_DST_BIT:										return "VK_IMAGE_USAGE_TRANSFER_DST_BIT";
            case VK_IMAGE_USAGE_SAMPLED_BIT:											return "VK_IMAGE_USAGE_SAMPLED_BIT";
            case VK_IMAGE_USAGE_STORAGE_BIT:											return "VK_IMAGE_USAGE_STORAGE_BIT";
            case VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT:									return "VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT:							return "VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT:								return "VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT:									return "VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT";
            case VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR:								return "VK_IMAGE_USAGE_VIDEO_DECODE_DST_BIT_KHR";
            case VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR:								return "VK_IMAGE_USAGE_VIDEO_DECODE_SRC_BIT_KHR";
            case VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR:								return "VK_IMAGE_USAGE_VIDEO_DECODE_DPB_BIT_KHR";
            case VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT:							return "VK_IMAGE_USAGE_FRAGMENT_DENSITY_MAP_BIT_EXT";
            case VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR:				return "VK_IMAGE_USAGE_FRAGMENT_SHADING_RATE_ATTACHMENT_BIT_KHR";
            case VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT:						return "VK_IMAGE_USAGE_ATTACHMENT_FEEDBACK_LOOP_BIT_EXT";
            case VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI:								return "VK_IMAGE_USAGE_INVOCATION_MASK_BIT_HUAWEI";
            case VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM:									return "VK_IMAGE_USAGE_SAMPLE_WEIGHT_BIT_QCOM";
            case VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM:							return "VK_IMAGE_USAGE_SAMPLE_BLOCK_MATCH_BIT_QCOM";
            default:																	return "Unknow image usage";
        }
    }

    const char* ToString(VkFormat format) noexcept
    {
        switch (format)
        {
            case VK_FORMAT_UNDEFINED: return "VK_FORMAT_UNDEFINED";
            case VK_FORMAT_R4G4_UNORM_PACK8: return "VK_FORMAT_R4G4_UNORM_PACK8";
            case VK_FORMAT_R4G4B4A4_UNORM_PACK16: return "VK_FORMAT_R4G4B4A4_UNORM_PACK16";
            case VK_FORMAT_B4G4R4A4_UNORM_PACK16: return "VK_FORMAT_B4G4R4A4_UNORM_PACK16";
            case VK_FORMAT_R5G6B5_UNORM_PACK16: return "VK_FORMAT_R5G6B5_UNORM_PACK16";
            case VK_FORMAT_B5G6R5_UNORM_PACK16: return "VK_FORMAT_B5G6R5_UNORM_PACK16";
            case VK_FORMAT_R5G5B5A1_UNORM_PACK16: return "VK_FORMAT_R5G5B5A1_UNORM_PACK16";
            case VK_FORMAT_B5G5R5A1_UNORM_PACK16: return "VK_FORMAT_B5G5R5A1_UNORM_PACK16";
            case VK_FORMAT_A1R5G5B5_UNORM_PACK16: return "VK_FORMAT_A1R5G5B5_UNORM_PACK16";
            case VK_FORMAT_R8_UNORM: return "VK_FORMAT_R8_UNORM";
            case VK_FORMAT_R8_SNORM: return "VK_FORMAT_R8_SNORM";
            case VK_FORMAT_R8_USCALED: return "VK_FORMAT_R8_USCALED";
            case VK_FORMAT_R8_SSCALED: return "VK_FORMAT_R8_SSCALED";
            case VK_FORMAT_R8_UINT: return "VK_FORMAT_R8_UINT";
            case VK_FORMAT_R8_SINT: return "VK_FORMAT_R8_SINT";
            case VK_FORMAT_R8_SRGB: return "VK_FORMAT_R8_SRGB";
            case VK_FORMAT_R8G8_UNORM: return "VK_FORMAT_R8G8_UNORM";
            case VK_FORMAT_R8G8_SNORM: return "VK_FORMAT_R8G8_SNORM";
            case VK_FORMAT_R8G8_USCALED: return "VK_FORMAT_R8G8_USCALED";
            case VK_FORMAT_R8G8_SSCALED: return "VK_FORMAT_R8G8_SSCALED";
            case VK_FORMAT_R8G8_UINT: return "VK_FORMAT_R8G8_UINT";
            case VK_FORMAT_R8G8_SINT: return "VK_FORMAT_R8G8_SINT";
            case VK_FORMAT_R8G8_SRGB: return "VK_FORMAT_R8G8_SRGB";
            case VK_FORMAT_R8G8B8_UNORM: return "VK_FORMAT_R8G8B8_UNORM";
            case VK_FORMAT_R8G8B8_SNORM: return "VK_FORMAT_R8G8B8_SNORM";
            case VK_FORMAT_R8G8B8_USCALED: return "VK_FORMAT_R8G8B8_USCALED";
            case VK_FORMAT_R8G8B8_SSCALED: return "VK_FORMAT_R8G8B8_SSCALED";
            case VK_FORMAT_R8G8B8_UINT: return "VK_FORMAT_R8G8B8_UINT";
            case VK_FORMAT_R8G8B8_SINT: return "VK_FORMAT_R8G8B8_SINT";
            case VK_FORMAT_R8G8B8_SRGB: return "VK_FORMAT_R8G8B8_SRGB";
            case VK_FORMAT_B8G8R8_UNORM: return "VK_FORMAT_B8G8R8_UNORM";
            case VK_FORMAT_B8G8R8_SNORM: return "VK_FORMAT_B8G8R8_SNORM";
            case VK_FORMAT_B8G8R8_USCALED: return "VK_FORMAT_B8G8R8_USCALED";
            case VK_FORMAT_B8G8R8_SSCALED: return "VK_FORMAT_B8G8R8_SSCALED";
            case VK_FORMAT_B8G8R8_UINT: return "VK_FORMAT_B8G8R8_UINT";
            case VK_FORMAT_B8G8R8_SINT: return "VK_FORMAT_B8G8R8_SINT";
            case VK_FORMAT_B8G8R8_SRGB: return "VK_FORMAT_B8G8R8_SRGB";
            case VK_FORMAT_R8G8B8A8_UNORM: return "VK_FORMAT_R8G8B8A8_UNORM";
            case VK_FORMAT_R8G8B8A8_SNORM: return "VK_FORMAT_R8G8B8A8_SNORM";
            case VK_FORMAT_R8G8B8A8_USCALED: return "VK_FORMAT_R8G8B8A8_USCALED";
            case VK_FORMAT_R8G8B8A8_SSCALED: return "VK_FORMAT_R8G8B8A8_SSCALED";
            case VK_FORMAT_R8G8B8A8_UINT: return "VK_FORMAT_R8G8B8A8_UINT";
            case VK_FORMAT_R8G8B8A8_SINT: return "VK_FORMAT_R8G8B8A8_SINT";
            case VK_FORMAT_R8G8B8A8_SRGB: return "VK_FORMAT_R8G8B8A8_SRGB";
            case VK_FORMAT_B8G8R8A8_UNORM: return "VK_FORMAT_B8G8R8A8_UNORM";
            case VK_FORMAT_B8G8R8A8_SNORM: return "VK_FORMAT_B8G8R8A8_SNORM";
            case VK_FORMAT_B8G8R8A8_USCALED: return "VK_FORMAT_B8G8R8A8_USCALED";
            case VK_FORMAT_B8G8R8A8_SSCALED: return "VK_FORMAT_B8G8R8A8_SSCALED";
            case VK_FORMAT_B8G8R8A8_UINT: return "VK_FORMAT_B8G8R8A8_UINT";
            case VK_FORMAT_B8G8R8A8_SINT: return "VK_FORMAT_B8G8R8A8_SINT";
            case VK_FORMAT_B8G8R8A8_SRGB: return "VK_FORMAT_B8G8R8A8_SRGB";
            case VK_FORMAT_A8B8G8R8_UNORM_PACK32: return "VK_FORMAT_A8B8G8R8_UNORM_PACK32";
            case VK_FORMAT_A8B8G8R8_SNORM_PACK32: return "VK_FORMAT_A8B8G8R8_SNORM_PACK32";
            case VK_FORMAT_A8B8G8R8_USCALED_PACK32: return "VK_FORMAT_A8B8G8R8_USCALED_PACK32";
            case VK_FORMAT_A8B8G8R8_SSCALED_PACK32: return "VK_FORMAT_A8B8G8R8_SSCALED_PACK32";
            case VK_FORMAT_A8B8G8R8_UINT_PACK32: return "VK_FORMAT_A8B8G8R8_UINT_PACK32";
            case VK_FORMAT_A8B8G8R8_SINT_PACK32: return "VK_FORMAT_A8B8G8R8_SINT_PACK32";
            case VK_FORMAT_A8B8G8R8_SRGB_PACK32: return "VK_FORMAT_A8B8G8R8_SRGB_PACK32";
            case VK_FORMAT_A2R10G10B10_UNORM_PACK32: return "VK_FORMAT_A2R10G10B10_UNORM_PACK32";
            case VK_FORMAT_A2R10G10B10_SNORM_PACK32: return "VK_FORMAT_A2R10G10B10_SNORM_PACK32";
            case VK_FORMAT_A2R10G10B10_USCALED_PACK32: return "VK_FORMAT_A2R10G10B10_USCALED_PACK32";
            case VK_FORMAT_A2R10G10B10_SSCALED_PACK32: return "VK_FORMAT_A2R10G10B10_SSCALED_PACK32";
            case VK_FORMAT_A2R10G10B10_UINT_PACK32: return "VK_FORMAT_A2R10G10B10_UINT_PACK32";
            case VK_FORMAT_A2R10G10B10_SINT_PACK32: return "VK_FORMAT_A2R10G10B10_SINT_PACK32";
            case VK_FORMAT_A2B10G10R10_UNORM_PACK32: return "VK_FORMAT_A2B10G10R10_UNORM_PACK32";
            case VK_FORMAT_A2B10G10R10_SNORM_PACK32: return "VK_FORMAT_A2B10G10R10_SNORM_PACK32";
            case VK_FORMAT_A2B10G10R10_USCALED_PACK32: return "VK_FORMAT_A2B10G10R10_USCALED_PACK32";
            case VK_FORMAT_A2B10G10R10_SSCALED_PACK32: return "VK_FORMAT_A2B10G10R10_SSCALED_PACK32";
            case VK_FORMAT_A2B10G10R10_UINT_PACK32: return "VK_FORMAT_A2B10G10R10_UINT_PACK32";
            case VK_FORMAT_A2B10G10R10_SINT_PACK32: return "VK_FORMAT_A2B10G10R10_SINT_PACK32";
            case VK_FORMAT_R16_UNORM: return "VK_FORMAT_R16_UNORM";
            case VK_FORMAT_R16_SNORM: return "VK_FORMAT_R16_SNORM";
            case VK_FORMAT_R16_USCALED: return "VK_FORMAT_R16_USCALED";
            case VK_FORMAT_R16_SSCALED: return "VK_FORMAT_R16_SSCALED";
            case VK_FORMAT_R16_UINT: return "VK_FORMAT_R16_UINT";
            case VK_FORMAT_R16_SINT: return "VK_FORMAT_R16_SINT";
            case VK_FORMAT_R16_SFLOAT: return "VK_FORMAT_R16_SFLOAT";
            case VK_FORMAT_R16G16_UNORM: return "VK_FORMAT_R16G16_UNORM";
            case VK_FORMAT_R16G16_SNORM: return "VK_FORMAT_R16G16_SNORM";
            case VK_FORMAT_R16G16_USCALED: return "VK_FORMAT_R16G16_USCALED";
            case VK_FORMAT_R16G16_SSCALED: return "VK_FORMAT_R16G16_SSCALED";
            case VK_FORMAT_R16G16_UINT: return "VK_FORMAT_R16G16_UINT";
            case VK_FORMAT_R16G16_SINT: return "VK_FORMAT_R16G16_SINT";
            case VK_FORMAT_R16G16_SFLOAT: return "VK_FORMAT_R16G16_SFLOAT";
            case VK_FORMAT_R16G16B16_UNORM: return "VK_FORMAT_R16G16B16_UNORM";
            case VK_FORMAT_R16G16B16_SNORM: return "VK_FORMAT_R16G16B16_SNORM";
            case VK_FORMAT_R16G16B16_USCALED: return "VK_FORMAT_R16G16B16_USCALED";
            case VK_FORMAT_R16G16B16_SSCALED: return "VK_FORMAT_R16G16B16_SSCALED";
            case VK_FORMAT_R16G16B16_UINT: return "VK_FORMAT_R16G16B16_UINT";
            case VK_FORMAT_R16G16B16_SINT: return "VK_FORMAT_R16G16B16_SINT";
            case VK_FORMAT_R16G16B16_SFLOAT: return "VK_FORMAT_R16G16B16_SFLOAT";
            case VK_FORMAT_R16G16B16A16_UNORM: return "VK_FORMAT_R16G16B16A16_UNORM";
            case VK_FORMAT_R16G16B16A16_SNORM: return "VK_FORMAT_R16G16B16A16_SNORM";
            case VK_FORMAT_R16G16B16A16_USCALED: return "VK_FORMAT_R16G16B16A16_USCALED";
            case VK_FORMAT_R16G16B16A16_SSCALED: return "VK_FORMAT_R16G16B16A16_SSCALED";
            case VK_FORMAT_R16G16B16A16_UINT: return "VK_FORMAT_R16G16B16A16_UINT";
            case VK_FORMAT_R16G16B16A16_SINT: return "VK_FORMAT_R16G16B16A16_SINT";
            case VK_FORMAT_R16G16B16A16_SFLOAT: return "VK_FORMAT_R16G16B16A16_SFLOAT";
            case VK_FORMAT_R32_UINT: return "VK_FORMAT_R32_UINT";
            case VK_FORMAT_R32_SINT: return "VK_FORMAT_R32_SINT";
            case VK_FORMAT_R32_SFLOAT: return "VK_FORMAT_R32_SFLOAT";
            case VK_FORMAT_R32G32_UINT: return "VK_FORMAT_R32G32_UINT";
            case VK_FORMAT_R32G32_SINT: return "VK_FORMAT_R32G32_SINT";
            case VK_FORMAT_R32G32_SFLOAT: return "VK_FORMAT_R32G32_SFLOAT";
            case VK_FORMAT_R32G32B32_UINT: return "VK_FORMAT_R32G32B32_UINT";
            case VK_FORMAT_R32G32B32_SINT: return "VK_FORMAT_R32G32B32_SINT";
            case VK_FORMAT_R32G32B32_SFLOAT: return "VK_FORMAT_R32G32B32_SFLOAT";
            case VK_FORMAT_R32G32B32A32_UINT: return "VK_FORMAT_R32G32B32A32_UINT";
            case VK_FORMAT_R32G32B32A32_SINT: return "VK_FORMAT_R32G32B32A32_SINT";
            case VK_FORMAT_R32G32B32A32_SFLOAT: return "VK_FORMAT_R32G32B32A32_SFLOAT";
            case VK_FORMAT_R64_UINT: return "VK_FORMAT_R64_UINT";
            case VK_FORMAT_R64_SINT: return "VK_FORMAT_R64_SINT";
            case VK_FORMAT_R64_SFLOAT: return "VK_FORMAT_R64_SFLOAT";
            case VK_FORMAT_R64G64_UINT: return "VK_FORMAT_R64G64_UINT";
            case VK_FORMAT_R64G64_SINT: return "VK_FORMAT_R64G64_SINT";
            case VK_FORMAT_R64G64_SFLOAT: return "VK_FORMAT_R64G64_SFLOAT";
            case VK_FORMAT_R64G64B64_UINT: return "VK_FORMAT_R64G64B64_UINT";
            case VK_FORMAT_R64G64B64_SINT: return "VK_FORMAT_R64G64B64_SINT";
            case VK_FORMAT_R64G64B64_SFLOAT: return "VK_FORMAT_R64G64B64_SFLOAT";
            case VK_FORMAT_R64G64B64A64_UINT: return "VK_FORMAT_R64G64B64A64_UINT";
            case VK_FORMAT_R64G64B64A64_SINT: return "VK_FORMAT_R64G64B64A64_SINT";
            case VK_FORMAT_R64G64B64A64_SFLOAT: return "VK_FORMAT_R64G64B64A64_SFLOAT";
            case VK_FORMAT_B10G11R11_UFLOAT_PACK32: return "VK_FORMAT_B10G11R11_UFLOAT_PACK32";
            case VK_FORMAT_E5B9G9R9_UFLOAT_PACK32: return "VK_FORMAT_E5B9G9R9_UFLOAT_PACK32";
            case VK_FORMAT_D16_UNORM: return "VK_FORMAT_D16_UNORM";
            case VK_FORMAT_X8_D24_UNORM_PACK32: return "VK_FORMAT_X8_D24_UNORM_PACK32";
            case VK_FORMAT_D32_SFLOAT: return "VK_FORMAT_D32_SFLOAT";
            case VK_FORMAT_S8_UINT: return "VK_FORMAT_S8_UINT";
            case VK_FORMAT_D16_UNORM_S8_UINT: return "VK_FORMAT_D16_UNORM_S8_UINT";
            case VK_FORMAT_D24_UNORM_S8_UINT: return "VK_FORMAT_D24_UNORM_S8_UINT";
            case VK_FORMAT_D32_SFLOAT_S8_UINT: return "VK_FORMAT_D32_SFLOAT_S8_UINT";
            case VK_FORMAT_BC1_RGB_UNORM_BLOCK: return "VK_FORMAT_BC1_RGB_UNORM_BLOCK";
            case VK_FORMAT_BC1_RGB_SRGB_BLOCK: return "VK_FORMAT_BC1_RGB_SRGB_BLOCK";
            case VK_FORMAT_BC1_RGBA_UNORM_BLOCK: return "VK_FORMAT_BC1_RGBA_UNORM_BLOCK";
            case VK_FORMAT_BC1_RGBA_SRGB_BLOCK: return "VK_FORMAT_BC1_RGBA_SRGB_BLOCK";
            case VK_FORMAT_BC2_UNORM_BLOCK: return "VK_FORMAT_BC2_UNORM_BLOCK";
            case VK_FORMAT_BC2_SRGB_BLOCK: return "VK_FORMAT_BC2_SRGB_BLOCK";
            case VK_FORMAT_BC3_UNORM_BLOCK: return "VK_FORMAT_BC3_UNORM_BLOCK";
            case VK_FORMAT_BC3_SRGB_BLOCK: return "VK_FORMAT_BC3_SRGB_BLOCK";
            case VK_FORMAT_BC4_UNORM_BLOCK: return "VK_FORMAT_BC4_UNORM_BLOCK";
            case VK_FORMAT_BC4_SNORM_BLOCK: return "VK_FORMAT_BC4_SNORM_BLOCK";
            case VK_FORMAT_BC5_UNORM_BLOCK: return "VK_FORMAT_BC5_UNORM_BLOCK";
            case VK_FORMAT_BC5_SNORM_BLOCK: return "VK_FORMAT_BC5_SNORM_BLOCK";
            case VK_FORMAT_BC6H_UFLOAT_BLOCK: return "VK_FORMAT_BC6H_UFLOAT_BLOCK";
            case VK_FORMAT_BC6H_SFLOAT_BLOCK: return "VK_FORMAT_BC6H_SFLOAT_BLOCK";
            case VK_FORMAT_BC7_UNORM_BLOCK: return "VK_FORMAT_BC7_UNORM_BLOCK";
            case VK_FORMAT_BC7_SRGB_BLOCK: return "VK_FORMAT_BC7_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK";
            case VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK: return "VK_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK";
            case VK_FORMAT_EAC_R11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11_UNORM_BLOCK";
            case VK_FORMAT_EAC_R11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11_SNORM_BLOCK";
            case VK_FORMAT_EAC_R11G11_UNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_UNORM_BLOCK";
            case VK_FORMAT_EAC_R11G11_SNORM_BLOCK: return "VK_FORMAT_EAC_R11G11_SNORM_BLOCK";
            case VK_FORMAT_ASTC_4x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_4x4_UNORM_BLOCK";
            case VK_FORMAT_ASTC_4x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_4x4_SRGB_BLOCK";
            case VK_FORMAT_ASTC_5x4_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x4_UNORM_BLOCK";
            case VK_FORMAT_ASTC_5x4_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x4_SRGB_BLOCK";
            case VK_FORMAT_ASTC_5x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_5x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_5x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_5x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_6x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_6x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_6x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_6x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_6x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_6x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_8x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_8x8_UNORM_BLOCK";
            case VK_FORMAT_ASTC_8x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_8x8_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x5_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x5_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x5_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x5_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x6_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x6_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x6_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x6_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x8_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x8_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x8_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x8_SRGB_BLOCK";
            case VK_FORMAT_ASTC_10x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_10x10_UNORM_BLOCK";
            case VK_FORMAT_ASTC_10x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_10x10_SRGB_BLOCK";
            case VK_FORMAT_ASTC_12x10_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x10_UNORM_BLOCK";
            case VK_FORMAT_ASTC_12x10_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x10_SRGB_BLOCK";
            case VK_FORMAT_ASTC_12x12_UNORM_BLOCK: return "VK_FORMAT_ASTC_12x12_UNORM_BLOCK";
            case VK_FORMAT_ASTC_12x12_SRGB_BLOCK: return "VK_FORMAT_ASTC_12x12_SRGB_BLOCK";
            case VK_FORMAT_G8B8G8R8_422_UNORM: return "VK_FORMAT_G8B8G8R8_422_UNORM";
            case VK_FORMAT_B8G8R8G8_422_UNORM: return "VK_FORMAT_B8G8R8G8_422_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM: return "VK_FORMAT_G8_B8_R8_3PLANE_420_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_420_UNORM: return "VK_FORMAT_G8_B8R8_2PLANE_420_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM: return "VK_FORMAT_G8_B8_R8_3PLANE_422_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_422_UNORM: return "VK_FORMAT_G8_B8R8_2PLANE_422_UNORM";
            case VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM: return "VK_FORMAT_G8_B8_R8_3PLANE_444_UNORM";
            case VK_FORMAT_R10X6_UNORM_PACK16: return "VK_FORMAT_R10X6_UNORM_PACK16";
            case VK_FORMAT_R10X6G10X6_UNORM_2PACK16: return "VK_FORMAT_R10X6G10X6_UNORM_2PACK16";
            case VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16: return "VK_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16";
            case VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16: return "VK_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16";
            case VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16: return "VK_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16: return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16: return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16: return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16: return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16: return "VK_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_R12X4_UNORM_PACK16: return "VK_FORMAT_R12X4_UNORM_PACK16";
            case VK_FORMAT_R12X4G12X4_UNORM_2PACK16: return "VK_FORMAT_R12X4G12X4_UNORM_2PACK16";
            case VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16: return "VK_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16";
            case VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16: return "VK_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16";
            case VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16: return "VK_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16: return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16: return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16: return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16: return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16: return "VK_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G16B16G16R16_422_UNORM: return "VK_FORMAT_G16B16G16R16_422_UNORM";
            case VK_FORMAT_B16G16R16G16_422_UNORM: return "VK_FORMAT_B16G16R16G16_422_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM: return "VK_FORMAT_G16_B16_R16_3PLANE_420_UNORM";
            case VK_FORMAT_G16_B16R16_2PLANE_420_UNORM: return "VK_FORMAT_G16_B16R16_2PLANE_420_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM: return "VK_FORMAT_G16_B16_R16_3PLANE_422_UNORM";
            case VK_FORMAT_G16_B16R16_2PLANE_422_UNORM: return "VK_FORMAT_G16_B16R16_2PLANE_422_UNORM";
            case VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM: return "VK_FORMAT_G16_B16_R16_3PLANE_444_UNORM";
            case VK_FORMAT_G8_B8R8_2PLANE_444_UNORM: return "VK_FORMAT_G8_B8R8_2PLANE_444_UNORM";
            case VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16: return "VK_FORMAT_G10X6_B10X6R10X6_2PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16: return "VK_FORMAT_G12X4_B12X4R12X4_2PLANE_444_UNORM_3PACK16";
            case VK_FORMAT_G16_B16R16_2PLANE_444_UNORM: return "VK_FORMAT_G16_B16R16_2PLANE_444_UNORM";
            case VK_FORMAT_A4R4G4B4_UNORM_PACK16: return "VK_FORMAT_A4R4G4B4_UNORM_PACK16";
            case VK_FORMAT_A4B4G4R4_UNORM_PACK16: return "VK_FORMAT_A4B4G4R4_UNORM_PACK16";
            case VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_4x4_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_5x4_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_5x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_6x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_6x6_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_8x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_8x6_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_8x8_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x5_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x6_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x8_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_10x10_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_12x10_SFLOAT_BLOCK";
            case VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK: return "VK_FORMAT_ASTC_12x12_SFLOAT_BLOCK";
            case VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC1_2BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC1_4BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC2_2BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG: return "VK_FORMAT_PVRTC2_4BPP_UNORM_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC1_2BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC1_4BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC2_2BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG: return "VK_FORMAT_PVRTC2_4BPP_SRGB_BLOCK_IMG";
            case VK_FORMAT_R16G16_S10_5_NV: return "VK_FORMAT_R16G16_S10_5_NV";
            default: return "Unknown vulkan format";
        }
    }
}

namespace std 
{
    std::size_t hash<VkDescriptorBufferInfo>::operator()(const VkDescriptorBufferInfo& key) const noexcept
    {
        std::size_t h = 0;
        Coust::Hash::Combine(h, key.buffer);
        Coust::Hash::Combine(h, key.range);
        Coust::Hash::Combine(h, key.offset);
        return h;
    }

    std::size_t hash<VkDescriptorImageInfo>::operator()(const VkDescriptorImageInfo& key) const noexcept
    {
        std::size_t h = 0;
        Coust::Hash::Combine(h, key.imageView);
        Coust::Hash::Combine(h, static_cast<std::underlying_type<VkImageLayout>::type>(key.imageLayout));
        Coust::Hash::Combine(h, key.sampler);
        return h;
    }

    std::size_t hash<VkWriteDescriptorSet>::operator()(const VkWriteDescriptorSet& key) const noexcept
    {
        std::size_t h = 0;
        Coust::Hash::Combine(h, key.dstSet);
        Coust::Hash::Combine(h, key.dstBinding);
        Coust::Hash::Combine(h, key.dstArrayElement);
        Coust::Hash::Combine(h, key.descriptorCount);
        Coust::Hash::Combine(h, key.descriptorType);
        switch (key.descriptorType)
        {
            case VK_DESCRIPTOR_TYPE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER:
            case VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE:
            case VK_DESCRIPTOR_TYPE_STORAGE_IMAGE:
            case VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT:
                for (uint32_t i = 0; i < key.descriptorCount; ++i)
                {
                    Coust::Hash::Combine(h, key.pImageInfo[i]);
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER:
                for (uint32_t i = 0; i < key.descriptorCount; ++i)
                {
                    Coust::Hash::Combine(h, key.pTexelBufferView[i]);
                }
                break;
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER:
            case VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC:
            case VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC:
                for (uint32_t i = 0; i < key.descriptorCount; ++i)
                {
                    Coust::Hash::Combine(h, key.pBufferInfo[i]);
                }
                break;
            default:
                COUST_CORE_WARN("The hash of descriptor write of type {} is not yet implemented", Coust::Render::VK::ToString(key.descriptorType));
        }
        return h;
    }
}