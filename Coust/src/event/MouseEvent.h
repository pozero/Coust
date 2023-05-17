#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "event/Event.h"

#include <utility>

namespace coust {
namespace detail {

class MouseButtonEventBase : public Event<"Input | Mouse"> {
protected:
    MouseButtonEventBase() = delete;

    MouseButtonEventBase(int bc) noexcept;

public:
    int get_button_code() const noexcept;

protected:
    int buttoncode;
};

}  // namespace detail

class MouseButtonPressEvent : public detail::MouseButtonEventBase {
public:
    MouseButtonPressEvent() = delete;

    MouseButtonPressEvent(MouseButtonPressEvent &&) noexcept = default;
    MouseButtonPressEvent(MouseButtonPressEvent const &) noexcept = default;
    MouseButtonPressEvent &operator=(
        MouseButtonPressEvent &&) noexcept = default;
    MouseButtonPressEvent &operator=(
        MouseButtonPressEvent const &) noexcept = default;

public:
    MouseButtonPressEvent(int bc) noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;
};

class MouseButtonReleaseEvent : public detail::MouseButtonEventBase {
public:
    MouseButtonReleaseEvent() = delete;

    MouseButtonReleaseEvent(MouseButtonReleaseEvent &&) noexcept = default;
    MouseButtonReleaseEvent(MouseButtonReleaseEvent const &) noexcept = default;
    MouseButtonReleaseEvent &operator=(
        MouseButtonReleaseEvent &&) noexcept = default;
    MouseButtonReleaseEvent &operator=(
        MouseButtonReleaseEvent const &) noexcept = default;

public:
    MouseButtonReleaseEvent(int bc) noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;
};

class MouseMoveEvent : public Event<"Input | Mouse"> {
public:
    MouseMoveEvent() = delete;
    MouseMoveEvent(MouseMoveEvent &&) = delete;
    MouseMoveEvent(MouseMoveEvent const &) = delete;
    MouseMoveEvent &operator=(MouseMoveEvent &&) = delete;
    MouseMoveEvent &operator=(MouseMoveEvent const &) = delete;

public:
    MouseMoveEvent(
        int32_t x, int32_t y, int32_t offset_x, int32_t offset_y) noexcept;

    std::pair<int32_t, int32_t> get_position() const noexcept;

    std::pair<int32_t, int32_t> get_offset() const noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;

private:
    int32_t pos_x, pos_y;
    int32_t rel_x, rel_y;
};

class MouseWheelEvent : public Event<"Input | Mouse"> {
public:
    MouseWheelEvent() = delete;
    MouseWheelEvent(MouseWheelEvent &&) = delete;
    MouseWheelEvent(MouseWheelEvent const &) = delete;
    MouseWheelEvent &operator=(MouseWheelEvent &&) = delete;
    MouseWheelEvent &operator=(MouseWheelEvent const &) = delete;

public:
    MouseWheelEvent(float offset_x, float offset_y) noexcept;

    std::pair<float, float> get_offset() const noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;

private:
    float rel_x, rel_y;
};

static_assert(detail::IsEvent<MouseButtonPressEvent>, "");
static_assert(detail::IsEvent<MouseButtonReleaseEvent>, "");
static_assert(detail::IsEvent<MouseMoveEvent>, "");
static_assert(detail::IsEvent<MouseWheelEvent>, "");

}  // namespace coust
