#include "pch.h"

#include "utils/Compiler.h"
#include "render/vulkan/utils/VulkanEnum2String.h"

namespace coust {
namespace render {

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wswitch")
std::string_view to_string_view(VkResult result) noexcept {
    switch (result) {
        case VK_SUCCESS:
            return "VK_SUCCESS";
        case VK_NOT_READY:
            return "VK_NOT_READY";
        case VK_TIMEOUT:
            return "VK_TIMEOUT";
        case VK_EVENT_SET:
            return "VK_EVENT_SET";
        case VK_EVENT_RESET:
            return "VK_EVENT_RESET";
        case VK_INCOMPLETE:
            return "VK_INCOMPLETE";
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return "VK_ERROR_OUT_OF_HOST_MEMORY";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
        case VK_ERROR_INITIALIZATION_FAILED:
            return "VK_ERROR_INITIALIZATION_FAILED";
        case VK_ERROR_DEVICE_LOST:
            return "VK_ERROR_DEVICE_LOST";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return "VK_ERROR_MEMORY_MAP_FAILED";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return "VK_ERROR_LAYER_NOT_PRESENT";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return "VK_ERROR_EXTENSION_NOT_PRESENT";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return "VK_ERROR_FEATURE_NOT_PRESENT";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return "VK_ERROR_INCOMPATIBLE_DRIVER";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return "VK_ERROR_TOO_MANY_OBJECTS";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return "VK_ERROR_FORMAT_NOT_SUPPORTED";
        case VK_ERROR_FRAGMENTED_POOL:
            return "VK_ERROR_FRAGMENTED_POOL";
        case VK_ERROR_UNKNOWN:
            return "VK_ERROR_UNKNOWN";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return "VK_ERROR_OUT_OF_POOL_MEMORY";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return "VK_ERROR_INVALID_EXTERNAL_HANDLE";
        case VK_ERROR_FRAGMENTATION:
            return "VK_ERROR_FRAGMENTATION";
        case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
            return "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS";
        case VK_PIPELINE_COMPILE_REQUIRED:
            return "VK_PIPELINE_COMPILE_REQUIRED";
        case VK_ERROR_SURFACE_LOST_KHR:
            return "VK_ERROR_SURFACE_LOST_KHR";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR";
        case VK_SUBOPTIMAL_KHR:
            return "VK_SUBOPTIMAL_KHR";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return "VK_ERROR_OUT_OF_DATE_KHR";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR";
        case VK_ERROR_VALIDATION_FAILED_EXT:
            return "VK_ERROR_VALIDATION_FAILED_EXT";
        case VK_ERROR_INVALID_SHADER_NV:
            return "VK_ERROR_INVALID_SHADER_NV";
        case VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR:
            return "VK_ERROR_IMAGE_USAGE_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PICTURE_LAYOUT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_OPERATION_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_FORMAT_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_PROFILE_CODEC_NOT_SUPPORTED_KHR";
        case VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR:
            return "VK_ERROR_VIDEO_STD_VERSION_NOT_SUPPORTED_KHR";
        case VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT:
            return "VK_ERROR_INVALID_DRM_FORMAT_MODIFIER_PLANE_LAYOUT_EXT";
        case VK_ERROR_NOT_PERMITTED_KHR:
            return "VK_ERROR_NOT_PERMITTED_KHR";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT";
        case VK_THREAD_IDLE_KHR:
            return "VK_THREAD_IDLE_KHR";
        case VK_THREAD_DONE_KHR:
            return "VK_THREAD_DONE_KHR";
        case VK_OPERATION_DEFERRED_KHR:
            return "VK_OPERATION_DEFERRED_KHR";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return "VK_OPERATION_NOT_DEFERRED_KHR";
        case VK_ERROR_COMPRESSION_EXHAUSTED_EXT:
            return "VK_ERROR_COMPRESSION_EXHAUSTED_EXT";
        case VK_RESULT_MAX_ENUM:
            return "VK_RESULT_MAX_ENUM";
    }
    ASSUME(0);
}

std::string_view to_string_view(VkObjectType obj_type) noexcept {
    switch (obj_type) {
        case VK_OBJECT_TYPE_UNKNOWN:
            return "UNKNOWN";
        case VK_OBJECT_TYPE_INSTANCE:
            return "INSTANCE";
        case VK_OBJECT_TYPE_PHYSICAL_DEVICE:
            return "PHYSICAL_DEVICE";
        case VK_OBJECT_TYPE_DEVICE:
            return "DEVICE";
        case VK_OBJECT_TYPE_QUEUE:
            return "QUEUE";
        case VK_OBJECT_TYPE_SEMAPHORE:
            return "SEMAPHORE";
        case VK_OBJECT_TYPE_COMMAND_BUFFER:
            return "COMMAND_BUFFER";
        case VK_OBJECT_TYPE_FENCE:
            return "FENCE";
        case VK_OBJECT_TYPE_DEVICE_MEMORY:
            return "DEVICE_MEMORY";
        case VK_OBJECT_TYPE_BUFFER:
            return "BUFFER";
        case VK_OBJECT_TYPE_IMAGE:
            return "IMAGE";
        case VK_OBJECT_TYPE_EVENT:
            return "EVENT";
        case VK_OBJECT_TYPE_QUERY_POOL:
            return "QUERY_POOL";
        case VK_OBJECT_TYPE_BUFFER_VIEW:
            return "BUFFER_VIEW";
        case VK_OBJECT_TYPE_IMAGE_VIEW:
            return "IMAGE_VIEW";
        case VK_OBJECT_TYPE_SHADER_MODULE:
            return "SHADER_MODULE";
        case VK_OBJECT_TYPE_PIPELINE_CACHE:
            return "PIPELINE_CACHE";
        case VK_OBJECT_TYPE_PIPELINE_LAYOUT:
            return "PIPELINE_LAYOUT";
        case VK_OBJECT_TYPE_RENDER_PASS:
            return "RENDER_PASS";
        case VK_OBJECT_TYPE_PIPELINE:
            return "PIPELINE";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET_LAYOUT:
            return "DESCRIPTOR_SET_LAYOUT";
        case VK_OBJECT_TYPE_SAMPLER:
            return "SAMPLER";
        case VK_OBJECT_TYPE_DESCRIPTOR_POOL:
            return "DESCRIPTOR_POOL";
        case VK_OBJECT_TYPE_DESCRIPTOR_SET:
            return "DESCRIPTOR_SET";
        case VK_OBJECT_TYPE_FRAMEBUFFER:
            return "FRAMEBUFFER";
        case VK_OBJECT_TYPE_COMMAND_POOL:
            return "COMMAND_POOL";
        case VK_OBJECT_TYPE_SAMPLER_YCBCR_CONVERSION:
            return "SAMPLER_YCBCR_CONVERSION";
        case VK_OBJECT_TYPE_DESCRIPTOR_UPDATE_TEMPLATE:
            return "DESCRIPTOR_UPDATE_TEMPLATE";
        case VK_OBJECT_TYPE_PRIVATE_DATA_SLOT:
            return "PRIVATE_DATA_SLOT";
        case VK_OBJECT_TYPE_SURFACE_KHR:
            return "SURFACE_KHR";
        case VK_OBJECT_TYPE_SWAPCHAIN_KHR:
            return "SWAPCHAIN_KHR";
        case VK_OBJECT_TYPE_DISPLAY_KHR:
            return "DISPLAY_KHR";
        case VK_OBJECT_TYPE_DISPLAY_MODE_KHR:
            return "DISPLAY_MODE_KHR";
        case VK_OBJECT_TYPE_DEBUG_REPORT_CALLBACK_EXT:
            return "DEBUG_REPORT_CALLBACK_EXT";
        case VK_OBJECT_TYPE_VIDEO_SESSION_KHR:
            return "VIDEO_SESSION_KHR";
        case VK_OBJECT_TYPE_VIDEO_SESSION_PARAMETERS_KHR:
            return "VIDEO_SESSION_PARAMETERS_KHR";
        case VK_OBJECT_TYPE_CU_MODULE_NVX:
            return "CU_MODULE_NVX";
        case VK_OBJECT_TYPE_CU_FUNCTION_NVX:
            return "CU_FUNCTION_NVX";
        case VK_OBJECT_TYPE_DEBUG_UTILS_MESSENGER_EXT:
            return "DEBUG_UTILS_MESSENGER_EXT";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_KHR:
            return "ACCELERATION_STRUCTURE_KHR";
        case VK_OBJECT_TYPE_VALIDATION_CACHE_EXT:
            return "VALIDATION_CACHE_EXT";
        case VK_OBJECT_TYPE_ACCELERATION_STRUCTURE_NV:
            return "ACCELERATION_STRUCTURE_NV";
        case VK_OBJECT_TYPE_PERFORMANCE_CONFIGURATION_INTEL:
            return "PERFORMANCE_CONFIGURATION_INTEL";
        case VK_OBJECT_TYPE_DEFERRED_OPERATION_KHR:
            return "DEFERRED_OPERATION_KHR";
        case VK_OBJECT_TYPE_INDIRECT_COMMANDS_LAYOUT_NV:
            return "INDIRECT_COMMANDS_LAYOUT_NV";
        case VK_OBJECT_TYPE_BUFFER_COLLECTION_FUCHSIA:
            return "BUFFER_COLLECTION_FUCHSIA";
        case VK_OBJECT_TYPE_MICROMAP_EXT:
            return "MICROMAP_EXT";
        case VK_OBJECT_TYPE_OPTICAL_FLOW_SESSION_NV:
            return "OPTICAL_FLOW_SESSION_NV";
        case VK_OBJECT_TYPE_MAX_ENUM:
            return "MAX_ENUM";
    }
    ASSUME(0);
}
WARNING_POP

}  // namespace render
}  // namespace coust
