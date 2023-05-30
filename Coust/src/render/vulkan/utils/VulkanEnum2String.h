#pragma once

#include "volk.h"

namespace coust {
namespace render {

std::string_view to_string_view(VkResult result) noexcept;

std::string_view to_string_view(VkObjectType obj_type) noexcept;

}  // namespace render
}  // namespace coust
