#include "pch.h"

#include "Coust/Render/Vulkan/VulkanUtils.h"
#include "Coust/Utils/Hash.h"

namespace Coust::Render::VK 
{
	bool IsDepthOnlyFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT || 
			   format == VK_FORMAT_D16_UNORM;
	}

	bool IsDepthStencilFormat(VkFormat format)
	{
		return format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		 	   format == VK_FORMAT_D24_UNORM_S8_UINT ||
			   format == VK_FORMAT_D16_UNORM_S8_UINT ||
			   IsDepthOnlyFormat(format);
	}

	const char* ToString(VkResult result)
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
	
	const char* ToString(VkObjectType objType)
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
	
    const char* ToString(VkAccessFlagBits bit)
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
    
    const char* ToString(VkShaderStageFlagBits bit)
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
	
	const char* ToString(VkCommandBufferLevel level)
	{
		switch (level)
		{
			case VK_COMMAND_BUFFER_LEVEL_PRIMARY:		return "PRIMARY";
			case VK_COMMAND_BUFFER_LEVEL_SECONDARY:		return "SECONDARY";
			default: 									return "Unknow command buffer level";
		}
	}
	
	const char* ToString(VkDescriptorType type)
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
	
	const char* ToString(VkBufferUsageFlagBits bit)
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

	const char* ToString(VkImageUsageFlagBits bit)
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
}

namespace std 
{
    std::size_t hash<VkDescriptorBufferInfo>::operator()(const VkDescriptorBufferInfo& key)
    {
        std::size_t h = 0;
        Coust::Hash::Combine(h, key.buffer);
        Coust::Hash::Combine(h, key.range);
        Coust::Hash::Combine(h, key.offset);
        return h;
    }

	std::size_t hash<VkDescriptorImageInfo>::operator()(const VkDescriptorImageInfo& key)
	{
		std::size_t h = 0;
		Coust::Hash::Combine(h, key.imageView);
		Coust::Hash::Combine(h, static_cast<std::underlying_type<VkImageLayout>::type>(key.imageLayout));
		Coust::Hash::Combine(h, key.sampler);
		return h;
	}

	std::size_t hash<VkWriteDescriptorSet>::operator()(const VkWriteDescriptorSet& key)
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