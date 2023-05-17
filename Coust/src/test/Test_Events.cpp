#include "pch.h"

#include "test/Test.h"

#include "core/Application.h"
#include "event/Event.h"
#include "event/EventBus.h"
#include "event/ApplicationEvent.h"
#include "event/KeyEvent.h"
#include "event/MouseEvent.h"

TEST_CASE("[Coust] [event] Events" * doctest::skip(true)) {
    using namespace coust;
    // Application app{};

    WindowResizeEvent resize_event{1280u, 800u};
    WindowCloseEvent close_event{};
    CHECK(resize_event.in_category("Application"));
    CHECK(close_event.in_category("Application"));
    // event_bus::publish(resize_event);
    // event_bus::publish(close_event);

    KeyPressEvent key_press_event{1, false};
    KeyReleaseEvent key_release_event{1};
    CHECK(key_press_event.in_category("Keyboard"));
    CHECK(key_press_event.in_category("Input"));
    CHECK(key_release_event.in_category("Keyboard"));
    CHECK(key_release_event.in_category("Input"));
    // event_bus::publish(key_press_event);
    // event_bus::publish(key_release_event);

    MouseButtonPressEvent mouse_button_press_event{1};
    MouseButtonReleaseEvent mouse_button_release_event{1};
    CHECK(mouse_button_press_event.in_category("Mouse"));
    CHECK(mouse_button_press_event.in_category("Input"));
    CHECK(mouse_button_release_event.in_category("Mouse"));
    CHECK(mouse_button_release_event.in_category("Input"));
    // event_bus::publish(mouse_button_press_event);
    // event_bus::publish(mouse_button_release_event);

    MouseMoveEvent mouse_move_event{1, 2, 3, 4};
    MouseWheelEvent mouse_wheel_event{.01f, .02f};
    CHECK(mouse_move_event.in_category("Mouse"));
    CHECK(mouse_move_event.in_category("Input"));
    CHECK(mouse_wheel_event.in_category("Mouse"));
    CHECK(mouse_wheel_event.in_category("Input"));
    // event_bus::publish(mouse_move_event);
    // event_bus::publish(mouse_wheel_event);

    auto const catch_window_resize = [](WindowResizeEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(resize_event, catch_window_resize));
    CHECK(!event_bus::dispatch(close_event, catch_window_resize));

    auto const catch_window_close = [](WindowCloseEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(close_event, catch_window_close));
    CHECK(!event_bus::dispatch(key_press_event, catch_window_close));

    auto const catch_key_press = [](KeyPressEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(key_press_event, catch_key_press));
    CHECK(!event_bus::dispatch(key_release_event, catch_key_press));

    auto const catch_key_release = [](KeyReleaseEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(key_release_event, catch_key_release));
    CHECK(!event_bus::dispatch(mouse_button_press_event, catch_key_release));

    auto const catch_mouse_button_press = [](MouseButtonPressEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(
        mouse_button_press_event, catch_mouse_button_press));
    CHECK(!event_bus::dispatch(
        mouse_button_release_event, catch_mouse_button_press));

    auto const catch_mouse_button_release = [](MouseButtonReleaseEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(
        mouse_button_release_event, catch_mouse_button_release));
    CHECK(!event_bus::dispatch(mouse_move_event, catch_mouse_button_release));

    auto const catch_mouse_move = [](MouseMoveEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(mouse_move_event, catch_mouse_move));
    CHECK(!event_bus::dispatch(mouse_wheel_event, catch_mouse_move));

    auto const catch_mouse_wheel = [](MouseWheelEvent&) {
        return true;
    };
    CHECK(event_bus::dispatch(mouse_wheel_event, catch_mouse_wheel));
    CHECK(!event_bus::dispatch(resize_event, catch_mouse_wheel));
}
