#include "pch.h"

#include "event/KeyEvent.h"

namespace coust {
namespace detail {

KeyEventBase::KeyEventBase(int sc) noexcept : scancode(sc) {
}

int KeyEventBase::get_scancode() const noexcept {
    return scancode;
}

}  // namespace detail

KeyPressEvent::KeyPressEvent(int sc, bool repeat) noexcept
    : detail::KeyEventBase(sc), repeated(repeat) {
}

bool KeyPressEvent::is_repeated() const noexcept {
    return repeated;
}

memory::string<DefaultAlloc> KeyPressEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(std::back_inserter(ret),
        "Key Press Event [Scancode: {}, Repeated: {}]", scancode, repeated);
    return ret;
}

KeyReleaseEvent::KeyReleaseEvent(int sc) noexcept : detail::KeyEventBase(sc) {
}

memory::string<DefaultAlloc> KeyReleaseEvent::to_string() const noexcept {
    memory::string<DefaultAlloc> ret{get_default_alloc()};
    std::format_to(
        std::back_inserter(ret), "Key Release Event [Scancode: {}]", scancode);
    return ret;
}

}  // namespace coust
