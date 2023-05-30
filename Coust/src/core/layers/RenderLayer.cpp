#include "pch.h"

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
}

void RenderLayer::on_ui_update() noexcept {
}

std::string_view RenderLayer::get_name() const noexcept {
    return m_name;
}

}  // namespace coust
