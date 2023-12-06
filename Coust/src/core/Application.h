#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "SDL_scancode.h"
WARNING_POP

#include "event/Event.h"
#include "event/ApplicationEvent.h"
#include "event/KeyEvent.h"
#include "utils/TypeName.h"
#include "utils/Log.h"
#include "utils/allocators/SmartPtr.h"
#include "core/layers/RenderLayer.h"
#include "core/Window.h"
#include "core/Memory.h"

#include <memory>

namespace coust {

class Application {
public:
    Application() noexcept;
    virtual ~Application() noexcept;

    void run() noexcept;

    void on_event(detail::IsEvent auto&& event) {
        event_bus::dispatch(event, [this](WindowCloseEvent&) -> bool {
            m_running = false;
            return true;
        });
        event_bus::dispatch(event, [this](KeyPressEvent& e) -> bool {
            if (e.get_scancode() == SDL_SCANCODE_ESCAPE) {
                m_running = false;
            }
            return true;
        });
        m_render_layer.on_event(event);
    }

    Window& get_window() noexcept;

public:
    static Application& get_instance() noexcept;

private:
    RenderLayer m_render_layer;

private:
    Window m_window;
    bool m_running = false;

private:
    static Application* s_instance;
};

memory::unique_ptr<Application, DefaultAlloc> create_application();

}  // namespace coust
