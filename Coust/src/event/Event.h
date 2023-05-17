#pragma once

#include "utils/Compiler.h"

#include <algorithm>

namespace coust {
namespace detail {

// https://ctrpeach.io/posts/cpp20-string-literal-template-parameters/
template <size_t N>
struct StringLiteral {
    static size_t constexpr size = N;

    constexpr StringLiteral(const char (&str)[N]) { std::copy_n(str, N, val); }

    char val[N];
};

template <size_t N>
StringLiteral(const char (&)[N]) -> StringLiteral<N>;

WARNING_PUSH
CLANG_DISABLE_WARNING("-Wunsafe-buffer-usage")
template <StringLiteral S>
consteval auto split_categories() {
    size_t constexpr category_count = std::invoke([]() {
        auto const& p = S.val;
        size_t count = 0;
        bool alnum = false, other = false;
        for (size_t i = 0; i < decltype(S)::size; ++i) {
            if (('a' <= p[i] && p[i] <= 'z') || ('A' <= p[i] && p[i] <= 'Z') ||
                ('0' <= p[i] && p[i] <= '9'))
                alnum = true;
            else if (alnum)
                other = true;
            if (alnum && other) {
                alnum = other = false;
                ++count;
            }
        }
        return count;
    });
    auto constexpr all_categories = std::invoke([]() {
        auto const& p = S.val;
        bool is_last_letter_alnum = false;
        std::array<std::string_view, category_count> ret;
        size_t current_word = 0;
        size_t this_word_begin = 0;
        for (size_t i = 0; i < decltype(S)::size; ++i) {
            bool isalnum = ('a' <= p[i] && p[i] <= 'z') ||
                           ('A' <= p[i] && p[i] <= 'Z') ||
                           ('0' <= p[i] && p[i] <= '9');
            // current word ends
            if (is_last_letter_alnum && !isalnum) {
                ret[current_word++] =
                    std::string_view{p + this_word_begin, i - this_word_begin};
            }
            // new word begins
            else if (!is_last_letter_alnum && isalnum) {
                this_word_begin = i;
            }
            is_last_letter_alnum = isalnum;
        }
        return ret;
    });
    return all_categories;
}
WARNING_POP

template <typename T>
concept IsEvent = requires(T& t) {
    { t.in_category(std::string_view{}) } noexcept -> std::same_as<bool>;
    { t.is_handled() } noexcept -> std::same_as<bool>;
    { t.set_handle(true) } noexcept -> std::same_as<void>;
    { t.to_string() } noexcept;
};

}  // namespace detail

template <detail::StringLiteral All_Categories>
class Event {
public:
    static bool in_category(std::string_view category) noexcept {
        return std::ranges::any_of(all_categories,
            [&category](auto const& c) { return c == category; });
    }

    bool is_handled() const noexcept { return m_handled; }

    void set_handle(bool b) noexcept { m_handled = b; }

protected:
    static auto constexpr all_categories =
        detail::split_categories<All_Categories>();

protected:
    bool m_handled = false;
};

// these function have to be put here to prevent infinite recursive include..
namespace event_bus {

template <detail::IsEvent E, typename F>
static constexpr bool dispatch(E&& event, F&& func)
    requires requires(std::decay_t<F>& f, std::decay_t<E>& e) {
        { f(std::forward<E>(e)) } -> std::same_as<bool>;
    }
{
    event.set_handle(func(std::forward<E>(event)));
    return true;
}

template <detail::IsEvent E, typename F>
static constexpr bool dispatch(
    [[maybe_unused]] E&& event, [[maybe_unused]] F&& func) {
    return false;
}

}  // namespace event_bus
}  // namespace coust
