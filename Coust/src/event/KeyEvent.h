#pragma once

#include "core/Memory.h"
#include "utils/allocators/StlContainer.h"
#include "event/Event.h"

namespace coust {
namespace detail {

class KeyEventBase : public Event<"Input | Keyboard"> {
protected:
    KeyEventBase() = delete;

    KeyEventBase(int sc) noexcept;

public:
    int get_scancode() const noexcept;

protected:
    int scancode = 0;
};

}  // namespace detail

class KeyPressEvent : public detail::KeyEventBase {
public:
    KeyPressEvent() = delete;

    KeyPressEvent(KeyPressEvent &&) noexcept = default;
    KeyPressEvent(KeyPressEvent const &) noexcept = default;
    KeyPressEvent &operator=(KeyPressEvent &&) noexcept = default;
    KeyPressEvent &operator=(KeyPressEvent const &) noexcept = default;

public:
    KeyPressEvent(int sc, bool repeat) noexcept;

    bool is_repeated() const noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;

private:
    bool repeated;
};

class KeyReleaseEvent : public detail::KeyEventBase {
public:
    KeyReleaseEvent() = delete;

    KeyReleaseEvent(KeyReleaseEvent &&) = delete;
    KeyReleaseEvent(KeyReleaseEvent const &) = delete;
    KeyReleaseEvent &operator=(KeyReleaseEvent &&) = delete;
    KeyReleaseEvent &operator=(KeyReleaseEvent const &) = delete;

public:
    KeyReleaseEvent(int sc) noexcept;

    memory::string<DefaultAlloc> to_string() const noexcept;
};

static_assert(detail::IsEvent<KeyPressEvent>, "");
static_assert(detail::IsEvent<KeyReleaseEvent>, "");

}  // namespace coust
