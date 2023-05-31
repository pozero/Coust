#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

std::string_view to_string_view(VkResult result) noexcept;

std::string_view to_string_view(VkObjectType obj_type) noexcept;

}  // namespace render
}  // namespace coust
