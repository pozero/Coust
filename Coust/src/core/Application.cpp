#include "pch.h"

#include "utils/Assert.h"

#include "render/vulkan/VulkanDriver.h"
#include "core/Application.h"
#include "core/Logger.h"

namespace coust {

Application* Application::s_instance = nullptr;

Application::Application() noexcept
    : m_render_layer("Vulkan Renderer"),
      m_window(Window::Config{"Coust", 800, 600, true}) {
    COUST_PANIC_IF(s_instance, "Can't create multiple application instance");
    s_instance = this;

    m_render_layer.on_attach();

    m_running = true;
}

Application::~Application() noexcept {
    m_render_layer.on_detach();

    s_instance = nullptr;
}

void Application::run() noexcept {
    TimeStep::time_t last_time, cur_time;
    last_time = TimeStep::clock_t::now();
    while (m_running) {
        cur_time = TimeStep::clock_t::now();
        TimeStep ts{last_time, cur_time};
        m_window.poll_events();
        m_render_layer.on_update(ts);
        last_time = cur_time;
    }
}

Window& Application::get_window() noexcept {
    return m_window;
}

Application& Application::get_instance() noexcept {
    return *s_instance;
}

}  // namespace coust
