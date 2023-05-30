#pragma once

#include "utils/AlignedStorage.h"
#include "render/vulkan/VulkanDriver.h"

namespace coust {
namespace render {

class Renderer {
public:
    Renderer(Renderer&&) = delete;
    Renderer(Renderer const&) = delete;
    Renderer& operator=(Renderer&&) = delete;
    Renderer& operator=(Renderer const&) = delete;

public:
    Renderer() noexcept;

private:
    AlignedStorage<VulkanDriver> m_vk_driver{};
};

}  // namespace render
}  // namespace coust
