#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "SDL_mouse.h"
#include "SDL_scancode.h"
WARNING_POP

#include "utils/AlignedStorage.h"
#include "event/MouseEvent.h"
#include "event/KeyEvent.h"
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

    void on_event([[maybe_unused]] detail::IsEvent auto&& event) noexcept {
        event_bus::dispatch(
            event, [this](MouseButtonPressEvent& mouse_press_event) -> bool {
                if (mouse_press_event.get_button_code() == SDL_BUTTON_LEFT) {
                    m_renderer.get().get_camera().set_holding(true);
                    return true;
                }
                return false;
            });
        event_bus::dispatch(event,
            [this](MouseButtonReleaseEvent& mouse_release_event) -> bool {
                if (mouse_release_event.get_button_code() == SDL_BUTTON_LEFT) {
                    m_renderer.get().get_camera().set_holding(false);
                    return true;
                }
                return false;
            });
        event_bus::dispatch(
            event, [this](MouseMoveEvent& mouse_move_event) -> bool {
                auto const [x, y] = mouse_move_event.get_position();
                m_renderer.get().get_camera().rotate(float(x), float(y));
                return true;
            });
        event_bus::dispatch(
            event, [this](MouseWheelEvent& mouse_wheel_event) -> bool {
                auto const [x, y] = mouse_wheel_event.get_offset();
                m_renderer.get().get_camera().zoom(y);
                return true;
            });
    }

    void on_update(TimeStep ts) noexcept;

    void on_ui_update() noexcept;

    std::string_view get_name() const noexcept;

private:
    AlignedStorage<render::Renderer> m_renderer;
    std::string_view m_name;
};

static_assert(detail::Layer<RenderLayer>);

}  // namespace coust
