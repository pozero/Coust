#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {
namespace detail {

template <typename T>
concept IsVulkanResource =
    std::is_arithmetic_v<decltype(std::decay_t<T>::object_type)> &&
    requires(T& t) {
        { std::is_pointer_v<decltype(t.get_handle())> };
        { std::is_same_v<decltype(t.get_device()), VkDevice> };
    };

VkResult tag_vkobj_with_name(
    std::string_view name, IsVulkanResource auto&& vk_res) noexcept {
    auto handle = vk_res.get_handle();
    uint64_t converted_handle = (uint64_t) handle;
    static_assert(sizeof(handle) == sizeof(uint64_t));
    using resource_type = std::remove_cvref_t<decltype(vk_res)>;
    VkObjectType type = (VkObjectType) resource_type::object_type;
    VkDebugUtilsObjectNameInfoEXT info{
        .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT,
        .objectType = type,
        .objectHandle = converted_handle,
        .pObjectName = name.data(),
    };
    VkDevice dev = vk_res.get_device();
    return vkSetDebugUtilsObjectNameEXT(dev, &info);
}

}  // namespace detail
}  // namespace render
}  // namespace coust
