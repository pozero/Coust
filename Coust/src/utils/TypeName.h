#pragma once

#include <type_traits>

namespace coust {
namespace detail {

template <std::size_t... Idxs>
constexpr auto substring_as_array(
    std::string_view str, std::index_sequence<Idxs...>) {
    return std::array{str[Idxs]..., '\0'};
}

// Compile time human-readable type name!
// https://bitwizeshift.github.io/posts/2021/03/09/getting-an-unmangled-type-name-at-compile-time/

template <typename T>
constexpr auto type_name_array() {
// It seems that compilation using clang under windows environment would also
// include the `_MSC_VER` macro, so we should check `__clang__` macro first.
#if defined(__clang__)
    constexpr auto prefix = std::string_view{"[T = "};
    constexpr auto suffix = std::string_view{"]"};
    constexpr auto function = std::string_view{__PRETTY_FUNCTION__};
#elif defined(_MSC_VER)
    constexpr auto prefix = std::string_view{"type_name_array<"};
    constexpr auto suffix = std::string_view{">(void)"};
    constexpr auto function = std::string_view{__FUNCSIG__};
#else
    #error Unsupported compiler
#endif
    constexpr auto start = function.find(prefix) + prefix.size();
    constexpr auto end = function.rfind(suffix);
    static_assert(start < end);

    constexpr auto name = function.substr(start, (end - start));
    return substring_as_array(name, std::make_index_sequence<name.size()>{});
}

template <typename T>
struct type_name_holder {
    static inline constexpr auto value = type_name_array<T>();
};

}  // namespace detail

template <typename T>
consteval auto type_name() -> std::string_view {
    constexpr auto& val = detail::type_name_holder<T>::value;
    return std::string_view{val.data(), val.size()};
}
}  // namespace coust
