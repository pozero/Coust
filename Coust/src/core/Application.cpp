#include "pch.h"

#include "utils/Assert.h"

#include "core/Application.h"
#include "core/Logger.h"

namespace coust {

Application* Application::s_instance = nullptr;

Application::Application() noexcept
    : m_window(Window::Config{"Coust", 800, 600, true}) {
    COUST_PANIC_IF(s_instance, "Can't create multiple application instance");
    s_instance = this;
    m_running = true;
}

Application::~Application() noexcept {
    s_instance = nullptr;
}

void Application::run() noexcept {
    while (m_running) {
        m_window.poll_events();
    }
}

Window& Application::get_window() noexcept {
    return m_window;
}

Application& Application::get_instance() noexcept {
    return *s_instance;
}

}  // namespace coust
