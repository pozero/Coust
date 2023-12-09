#include "pch.h"

#include "render/vulkan/VulkanShaderPool.h"

namespace coust {
namespace render {

VulkanShaderPool::VulkanShaderPool(VkDevice dev) noexcept : m_dev(dev) {
}

void VulkanShaderPool::reset() noexcept {
    m_shader_modules.clear();
}

const VulkanShaderModule *VulkanShaderPool::get_shader(
    VulkanShaderModule::Param const &param) noexcept {
    auto iter = m_shader_modules.find(param);
    if (iter != m_shader_modules.end()) {
        return iter.mapped().get();
    } else {
        auto shader_module = memory::allocate_unique<VulkanShaderModule>(
            get_default_alloc(), m_dev, param);
        auto [emplace_iter, success] =
            m_shader_modules.emplace(param, std::move(shader_module));
        COUST_ASSERT(success, "");
        return emplace_iter.mapped().get();
    }
}

}  // namespace render
}  // namespace coust
