#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "event/Event.h"

#include <utility>

namespace coust {

class WindowResizeEvent : public Event<"Application"> {
public:
    WindowResizeEvent() = delete;

    WindowResizeEvent(WindowResizeEvent&&) noexcept = default;
    WindowResizeEvent(WindowResizeEvent const&) noexcept = default;
    WindowResizeEvent& operator=(WindowResizeEvent&&) noexcept = default;
    WindowResizeEvent& operator=(WindowResizeEvent const&) noexcept = default;

public:
    WindowResizeEvent(int32_t w, int32_t h) noexcept;

    std::pair<int32_t, int32_t> get_extent() const noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;

private:
    int32_t width, height;
};

class WindowCloseEvent : public Event<"Application"> {
public:
    WindowCloseEvent() noexcept = default;
    WindowCloseEvent(WindowCloseEvent&&) noexcept = default;
    WindowCloseEvent(WindowCloseEvent const&) noexcept = default;
    WindowCloseEvent& operator=(WindowCloseEvent&&) noexcept = default;
    WindowCloseEvent& operator=(WindowCloseEvent const&) noexcept = default;

public:
    memory::string<DefaultAlloc> to_string() const noexcept;
};

static_assert(detail::IsEvent<WindowResizeEvent>, "");
static_assert(detail::IsEvent<WindowCloseEvent>, "");

}  // namespace coust
