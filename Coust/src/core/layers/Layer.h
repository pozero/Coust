#pragma once

#include "event/ApplicationEvent.h"
#include "utils/TimeStep.h"

#include <type_traits>

namespace coust {
namespace detail {

template <typename T>
concept Layer = std::constructible_from<T, std::string_view> &&
                requires(T& t, WindowCloseEvent& wce, TimeStep const& ts) {
                    { t.on_attach() } noexcept -> std::same_as<void>;
                    { t.on_detach() } noexcept -> std::same_as<void>;
                    { t.on_event(wce) } noexcept -> std::same_as<void>;
                    { t.on_update(ts) } noexcept -> std::same_as<void>;
                    { t.on_ui_update() } noexcept -> std::same_as<void>;
                    { t.get_name() } noexcept -> std::same_as<std::string_view>;
                };

}  // namespace detail
}  // namespace coust
