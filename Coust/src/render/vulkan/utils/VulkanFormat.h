#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

bool is_depth_only_format(VkFormat format) noexcept;

bool is_depth_stencil_format(VkFormat format) noexcept;

VkFormat unpack_srgb_format(VkFormat srgb_format) noexcept;

uint32_t get_byte_per_pixel_from_format(VkFormat format) noexcept;

VkImageMemoryBarrier2 image_blit_transition(
    VkImageMemoryBarrier2 barrier) noexcept;

void transition_image_layout(
    VkCommandBuffer cmdbuf, VkImageMemoryBarrier2 barrier) noexcept;

}  // namespace render
}  // namespace coust
