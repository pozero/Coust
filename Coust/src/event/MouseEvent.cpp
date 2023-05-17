#include "pch.h"

#include "event/MouseEvent.h"

namespace coust {
namespace detail {

MouseButtonEventBase::MouseButtonEventBase(int bc) noexcept : buttoncode(bc) {
}

int MouseButtonEventBase::get_button_code() const noexcept {
    return buttoncode;
}

}  // namespace detail

MouseButtonPressEvent::MouseButtonPressEvent(int bc) noexcept
    : detail::MouseButtonEventBase(bc) {
}

memory::string<DefaultAlloc> MouseButtonPressEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret),
        "Mouse Button Press Event [Button Code: {}]", buttoncode);
    return ret;
}

MouseButtonReleaseEvent::MouseButtonReleaseEvent(int bc) noexcept
    : detail::MouseButtonEventBase(bc) {
}

memory::string<DefaultAlloc> MouseButtonReleaseEvent::to_string()
    const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret),
        "Mouse Button Release Event [Button Code: {}]", buttoncode);
    return ret;
}

MouseMoveEvent::MouseMoveEvent(
    int32_t x, int32_t y, int32_t offset_x, int32_t offset_y) noexcept
    : pos_x(x), pos_y(y), rel_x(offset_x), rel_y(offset_y) {
}

std::pair<int32_t, int32_t> MouseMoveEvent::get_position() const noexcept {
    return std::make_pair(pos_x, pos_y);
}

std::pair<int32_t, int32_t> MouseMoveEvent::get_offset() const noexcept {
    return std::make_pair(rel_x, rel_y);
}

memory::string<DefaultAlloc> MouseMoveEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret),
        "Mouse Move Event [New Pos: {}, {}, Offset: {}, {}]", pos_x, pos_y,
        rel_x, rel_y);
    return ret;
}

MouseWheelEvent::MouseWheelEvent(float offset_x, float offset_y) noexcept
    : rel_x(offset_x), rel_y(offset_y) {
}

std::pair<float, float> MouseWheelEvent::get_offset() const noexcept {
    return std::make_pair(rel_x, rel_y);
}

memory::string<DefaultAlloc> MouseWheelEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret),
        "Mouse Wheel Event [Offset: {}, {}]", rel_x, rel_y);
    return ret;
}

}  // namespace coust
