#include "pch.h"

#include "render/vulkan/utils/VulkanTagger.h"
#include "render/vulkan/VulkanRenderPass.h"

namespace coust {
namespace render {

VkDevice VulkanRenderPass::get_device() const noexcept {
    return m_dev;
}

VkRenderPass VulkanRenderPass::get_handle() const noexcept {
    return m_handle;
}

static_assert(detail::IsVulkanResource<VulkanRenderPass>);

}  // namespace render
}  // namespace coust
