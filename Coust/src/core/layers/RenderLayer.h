#pragma once

#include "utils/AlignedStorage.h"
#include "core/layers/Layer.h"
#include "render/Renderer.h"

namespace coust {

class RenderLayer {
public:
    RenderLayer(RenderLayer&&) = delete;
    RenderLayer(RenderLayer const&) = delete;
    RenderLayer& operator=(RenderLayer&&) = delete;
    RenderLayer& operator=(RenderLayer const&) = delete;

public:
    RenderLayer(std::string_view name) noexcept;

    void on_attach() noexcept;

    void on_detach() noexcept;

    void on_event([[maybe_unused]] detail::IsEvent auto&& event) noexcept {}

    void on_update(TimeStep const& ts) noexcept;

    void on_ui_update() noexcept;

    std::string_view get_name() const noexcept;

private:
    AlignedStorage<render::Renderer> m_renderer;
    std::string_view m_name;
};

static_assert(detail::Layer<RenderLayer>);

}  // namespace coust
