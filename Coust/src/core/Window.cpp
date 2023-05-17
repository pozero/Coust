#include "pch.h"

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "SDL.h"
#include "SDL_events.h"
#include "SDL_vulkan.h"
WARNING_POP

#include "event/Event.h"
#include "event/EventBus.h"
#include "event/ApplicationEvent.h"
#include "event/KeyEvent.h"
#include "event/MouseEvent.h"
#include "utils/Assert.h"
#include "core/Window.h"

namespace coust {

std::atomic_flag Window::s_sdl_initialization_flag = ATOMIC_FLAG_INIT;

Window::Window(Config const& config) noexcept : m_window_prop(config) {
    COUST_PANIC_IF(s_sdl_initialization_flag.test(std::memory_order_relaxed),
        "Coust doesn't support more than one window");
    COUST_PANIC_IF(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0,
        "Can't initialize SDL, SDL_Init reported: {}", SDL_GetError());
    s_sdl_initialization_flag.test_and_set(std::memory_order_release);

    SDL_WindowFlags const flags = std::invoke([&config]() {
        int f = SDL_WINDOW_VULKAN;
        if (config.resizeable)
            f |= SDL_WINDOW_RESIZABLE;
        return (SDL_WindowFlags) f;
    });
    int const x = SDL_WINDOWPOS_CENTERED;
    int const y = SDL_WINDOWPOS_CENTERED;
    m_window = SDL_CreateWindow(config.title.data(), x, y, (int) config.width,
        (int) config.height, flags);
    COUST_PANIC_IF(m_window == nullptr,
        "Can't create SDL window, SDL_CreateWindowWithPosition reported: {}",
        SDL_GetError());
}

Window::~Window() noexcept {
    if (s_sdl_initialization_flag.test(std::memory_order_relaxed)) {
        s_sdl_initialization_flag.clear(std::memory_order_release);
        SDL_DestroyWindow(m_window);
        SDL_Quit();
    }
}

bool Window::create_vksurface(
    VkInstance vk_instance, VkSurfaceKHR* out_vk_surface) const noexcept {
    return SDL_Vulkan_CreateSurface(m_window, vk_instance, out_vk_surface) ==
           SDL_TRUE;
}

bool Window::get_required_vkinstance_extension(
    uint32_t* out_count, const char** out_extension_names) const noexcept {
    return SDL_Vulkan_GetInstanceExtensions(
               m_window, out_count, out_extension_names) == SDL_TRUE;
}

void Window::poll_events() noexcept {
    auto const process_window_event = [](SDL_Event const& e) {
        auto const window_e_type = e.window.event;
        switch (window_e_type) {
            case SDL_WINDOWEVENT_RESIZED:
                event_bus::publish(
                    WindowResizeEvent{e.window.data1, e.window.data2});
                break;
            case SDL_WINDOWEVENT_CLOSE:
                event_bus::publish(WindowCloseEvent{});
                break;
        }
    };

    SDL_Event event{};
    while (SDL_PollEvent(&event)) {
        auto const e_type = event.type;
        switch (e_type) {
            case SDL_WINDOWEVENT:
                process_window_event(event);
                break;
            case SDL_KEYDOWN:
                event_bus::publish(KeyPressEvent{
                    event.key.keysym.scancode, (bool) event.key.repeat});
                break;
            case SDL_KEYUP:
                event_bus::publish(KeyReleaseEvent{event.key.keysym.scancode});
                break;
            case SDL_MOUSEMOTION:
                event_bus::publish(MouseMoveEvent{event.motion.x,
                    event.motion.y, event.motion.xrel, event.motion.yrel});
                break;
            case SDL_MOUSEBUTTONDOWN:
                event_bus::publish(MouseButtonPressEvent{event.button.button});
                break;
            case SDL_MOUSEBUTTONUP:
                event_bus::publish(
                    MouseButtonReleaseEvent{event.button.button});
                break;
            case SDL_MOUSEWHEEL:
                event_bus::publish(MouseWheelEvent{
                    event.wheel.preciseX, event.wheel.preciseY});
                break;
        }
    }
}

}  // namespace coust
