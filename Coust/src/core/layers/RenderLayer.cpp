#include "pch.h"

#include "utils/filesystem/FileIO.h"
#include "render/vulkan/VulkanDriver.h"
#include "core/layers/RenderLayer.h"

namespace coust {

RenderLayer::RenderLayer(std::string_view name) noexcept : m_name(name) {
}

void RenderLayer::on_attach() noexcept {
    m_renderer.initialize();
}

void RenderLayer::on_detach() noexcept {
}

void RenderLayer::on_update([[maybe_unused]] TimeStep const& ts) noexcept {
    m_renderer.get().begin_frame();
    std::filesystem::path gltf_path =
        file::get_absolute_path_from("Coust", "asset", "test", "minimal.gltf");
    std::filesystem::path vert_path =
        file::get_absolute_path_from("Coust", "shader", "test", "minimal.vert");
    std::filesystem::path frag_path =
        file::get_absolute_path_from("Coust", "shader", "test", "minimal.frag");
    m_renderer.get().render(gltf_path, vert_path, frag_path);
    m_renderer.get().end_frame();
}

void RenderLayer::on_ui_update() noexcept {
}

std::string_view RenderLayer::get_name() const noexcept {
    return m_name;
}

}  // namespace coust
