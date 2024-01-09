#pragma once

#include "utils/AlignedStorage.h"
#include "render/vulkan/VulkanDriver.h"
#include "render/camera/FPSCamera.h"

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

    void prepare(std::filesystem::path transformation_comp_shader_path,
        std::filesystem::path gltf_path, std::filesystem::path vert_shader_path,
        std::filesystem::path frag_shader_path) noexcept;

    void begin_frame() noexcept;

    void render() noexcept;

    void end_frame() noexcept;

    FPSCamera& get_camera() noexcept;

private:
    AlignedStorage<VulkanDriver> m_vk_driver{};

    const VulkanRenderTarget* m_attached_render_target = nullptr;

    memory::vector<MeshAggregate, DefaultAlloc> m_gltfes{get_default_alloc()};

    memory::vector<VulkanVertexIndexBuffer, DefaultAlloc> m_vertex_index_bufes{
        get_default_alloc()};

    memory::vector<VulkanTransformationBuffer, DefaultAlloc>
        m_transformation_bufes{get_default_alloc()};

    memory::vector<VulkanMaterialBuffer, DefaultAlloc> m_material_bufes{
        get_default_alloc()};

    memory::robin_map_nested<memory::string<DefaultAlloc>, uint32_t,
        DefaultAlloc>
        m_path_to_idx{get_default_alloc()};

    FPSCamera m_camera;

    uint32_t m_cur_idx;
};

}  // namespace render
}  // namespace coust
