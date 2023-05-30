#pragma once

#include "utils/Assert.h"
#include "render/vulkan/utils/VulkanEnum2String.h"

#define COUST_VK_CHECK(func, ...)                                              \
    do {                                                                       \
        VkResult res = func;                                                   \
        COUST_PANIC_IF_NOT(res == VK_SUCCESS,                                  \
            "Vulkan Call {} returned {}\n\t{}", #func,                         \
            coust::render::to_string_view(res), fmt::format(__VA_ARGS__));     \
    } while (false)
