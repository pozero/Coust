#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

uint32_t constexpr COUST_VULKAN_API_VERSION = VK_API_VERSION_1_3;

}  // namespace render
}  // namespace coust
