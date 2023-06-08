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

    ~Renderer() noexcept;

    void begin_frame() noexcept;

    void render(std::filesystem::path gltf_path,
        std::filesystem::path vert_shader_path,
        std::filesystem::path frag_shader_path) noexcept;

    void end_frame() noexcept;

private:
    AlignedStorage<VulkanDriver> m_vk_driver{};

    const VulkanRenderTarget* m_attached_render_target = nullptr;

    memory::vector<MeshAggregate, DefaultAlloc> m_gltfes{get_default_alloc()};

    memory::vector<VulkanVertexIndexBuffer, DefaultAlloc> m_vertex_index_bufes{
        get_default_alloc()};

    memory::robin_map_nested<memory::string<DefaultAlloc>, uint32_t,
        DefaultAlloc>
        m_path_to_idx{get_default_alloc()};
};

}  // namespace render
}  // namespace coust
