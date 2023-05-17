#pragma once

#include "event/Event.h"
#include "event/ApplicationEvent.h"
#include "utils/TypeName.h"
#include "utils/Log.h"
#include "utils/allocators/SmartPtr.h"
#include "core/Window.h"
#include "core/Memory.h"

#include <memory>

namespace coust {

class Application {
public:
    Application() noexcept;
    virtual ~Application() noexcept;

    void run() noexcept;

    template <detail::IsEvent E>
    void on_event(E&& event) {
        COUST_INFO("{}: {}", type_name<std::decay_t<E>>(), event.to_string());
        event_bus::dispatch(event, [this](WindowCloseEvent&) {
            m_running = false;
            return true;
        });
    }

    Window& get_window() noexcept;

public:
    static Application& get_instance() noexcept;

private:
    Window m_window;
    bool m_running = false;

private:
    static Application* s_instance;
};

memory::unique_ptr<Application, DefaultAlloc> create_application();

}  // namespace coust
