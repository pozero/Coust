#include "pch.h"

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "SDL_scancode.h"
WARNING_POP

#include "core/Application.h"
#include "utils/filesystem/FileIO.h"
#include "core/layers/RenderLayer.h"

namespace coust {

RenderLayer::RenderLayer(std::string_view name) noexcept : m_name(name) {
}

void RenderLayer::on_attach() noexcept {
    m_renderer.initialize();
}

void RenderLayer::on_detach() noexcept {
}

void RenderLayer::on_update(TimeStep ts) noexcept {
    std::filesystem::path gltf_path = file::get_absolute_path_from(
        "Coust", "asset", "test", "Sponza", "glTF", "Sponza.gltf");
    std::filesystem::path vert_path =
        file::get_absolute_path_from("Coust", "shader", "test", "minimal.vert");
    std::filesystem::path frag_path =
        file::get_absolute_path_from("Coust", "shader", "test", "minimal.frag");
    std::filesystem::path comp_path = file::get_absolute_path_from(
        "Coust", "shader", "test", "tran_calc.comp");
    WARNING_PUSH
    CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
    const uint8_t* keyboard =
        Application::get_instance().get_window().get_keyboard_state();
    if (keyboard[SDL_SCANCODE_W]) {
        m_renderer.get().get_camera().move_forward(ts);
    }
    if (keyboard[SDL_SCANCODE_S]) {
        m_renderer.get().get_camera().move_backward(ts);
    }
    if (keyboard[SDL_SCANCODE_A]) {
        m_renderer.get().get_camera().move_left(ts);
    }
    if (keyboard[SDL_SCANCODE_D]) {
        m_renderer.get().get_camera().move_right(ts);
    }
    WARNING_POP
    m_renderer.get().prepare(comp_path, gltf_path, vert_path, frag_path);
    m_renderer.get().begin_frame();
    m_renderer.get().render();
    m_renderer.get().end_frame();
}

void RenderLayer::on_ui_update() noexcept {
}

std::string_view RenderLayer::get_name() const noexcept {
    return m_name;
}

}  // namespace coust
