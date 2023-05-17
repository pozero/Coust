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
        uint32_t width;
        uint32_t height;
        bool resizeable;
    };

public:
    /* common */
    Window(Config const& config) noexcept;

    ~Window() noexcept;

    struct SDL_Window* get_handle() const noexcept { return m_window; }

    std::pair<uint32_t, uint32_t> get_size() const noexcept {
        return std::make_pair(m_window_prop.width, m_window_prop.height);
    }

    void poll_events() noexcept;
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
