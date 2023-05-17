#include "pch.h"

#include "event/ApplicationEvent.h"

namespace coust {
WindowResizeEvent::WindowResizeEvent(int32_t w, int32_t h) noexcept
    : width(w), height(h) {
}

std::pair<int32_t, int32_t> WindowResizeEvent::get_extent() const noexcept {
    return std::make_pair(width, height);
}

memory::string<DefaultAlloc> WindowResizeEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret),
        "Window Resizing Event [Width {}, Height {}]", width, height);
    return ret;
}

memory::string<DefaultAlloc> WindowCloseEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret), "Window Closing Event []");
    return ret;
}

}  // namespace coust
