#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "utils/allocators/SmartPtr.h"
#include "render/vulkan/VulkanShader.h"

namespace coust {
namespace render {

class VulkanShaderPool {
public:
    VulkanShaderPool() = delete;
    VulkanShaderPool(VulkanShaderPool &&) = delete;
    VulkanShaderPool(VulkanShaderPool const &) = delete;
    VulkanShaderPool &operator=(VulkanShaderPool &&) = delete;
    VulkanShaderPool &operator=(VulkanShaderPool const &) = delete;

public:
    VulkanShaderPool(VkDevice dev) noexcept;

    void reset() noexcept;

    VulkanShaderModule *get_shader(
        VulkanShaderModule::Param const &param) noexcept;

private:
    VkDevice m_dev = VK_NULL_HANDLE;

    memory::robin_map<VulkanShaderModule::Param,
        memory::unique_ptr<VulkanShaderModule, DefaultAlloc>, DefaultAlloc>
        m_shader_modules{get_default_alloc()};
};

}  // namespace render
}  // namespace coust
