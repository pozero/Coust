#pragma once

namespace coust {

template <typename T>
constexpr void hash_combine(std::size_t& seed, T const& val)
    requires requires(T const& t) {
        { std::hash<T>{}(t) } -> std::same_as<std::size_t>;
    }
{
    std::size_t hash = std::hash<T>{}(val);
    // from glm
    hash += 0x9e3779b9 + (seed << 6) + (seed >> 2);
    seed ^= hash;
}

template <typename T>
constexpr std::size_t calc_std_hash(T const& key)
    requires requires(T const& t) {
        { std::hash<T>{}(t) } -> std::same_as<std::size_t>;
    }
{
    using type = std::remove_cvref_t<T>;
    return std::hash<type>{}(key);
}

}  // namespace coust
