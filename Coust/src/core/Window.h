#pragma once

#include <cstdint>
#include <atomic>

// maybe can find better way to declare them?
typedef struct VkInstance_T* VkInstance;
typedef struct VkSurfaceKHR_T* VkSurfaceKHR;

namespace coust {

class Window {
public:
    struct Config {
        std::string_view title;
        int width;
        int height;
        bool resizeable;
    };

public:
    /* common */
    Window(Config const& config) noexcept;

    ~Window() noexcept;

    struct SDL_Window* get_handle() const noexcept { return m_window; }

    std::pair<int, int> get_size() noexcept;

    std::pair<int, int> get_drawable_size() const noexcept;

    void poll_events() noexcept;

    const uint8_t* get_keyboard_state() const noexcept;
    /* common */

public:
    /* vulkan */
    bool create_vksurface(
        VkInstance vk_instance, VkSurfaceKHR* out_vk_surface) const noexcept;

    bool get_required_vkinstance_extension(
        uint32_t* out_count, const char** out_extension_names) const noexcept;
    /* vulkan */

private:
    static std::atomic_flag s_sdl_initialization_flag;

private:
    struct SDL_Window* m_window = nullptr;
    Config m_window_prop;
};

}  // namespace coust
