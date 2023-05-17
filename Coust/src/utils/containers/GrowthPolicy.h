#pragma once

#include "utils/Assert.h"

#include <type_traits>

namespace coust {
namespace container {
namespace detail {

template <typename T>
concept growth_policy = std::constructible_from<T, size_t&> && requires(T& t) {
    { t.hash_to_index(size_t{}) } noexcept -> std::same_as<size_t>;
    { t.grow() } noexcept -> std::same_as<size_t>;
    { t.max() } noexcept -> std::same_as<size_t>;
    { t.clear() } noexcept -> std::same_as<void>;
    { t.next_idx(size_t{}) } noexcept -> std::same_as<size_t>;
};

template <size_t Grwoth_Factor>
class power_of_two_growth {
private:
    static size_t constexpr is_power_of_two(size_t val) {
        return val != 0 && (val & (val - 1)) == 0;
    }

    static size_t constexpr round_up_to_power_of_two(size_t val) {
        if (val == 0)
            return 1;
        else if (is_power_of_two(val))
            return val;
        --val;
        for (size_t i = 1; i < sizeof(size_t) * 8; i = i << 1) {
            val |= val >> i;
        }
        return val + 1;
    }

public:
    static_assert(Grwoth_Factor >= 2 && is_power_of_two(Grwoth_Factor),
        "Growth factor must be 2 ^ k, where k >= 1");
    static size_t constexpr MIN_COUNT = 2;

public:
    power_of_two_growth(size_t& inout_min_count) noexcept {
        COUST_ASSERT(inout_min_count < max(),
            "the count reaches its limit, growth failed");
        if (inout_min_count > 0) {
            inout_min_count = round_up_to_power_of_two(inout_min_count);
            m_mask = inout_min_count - 1;
        } else {
            m_mask = 0u;
        }
    }

    // modulo with power of 2 as divisor, so we can perform bit operation
    size_t hash_to_index(size_t hash) const noexcept { return hash & m_mask; }

    size_t grow() const noexcept {
        COUST_PANIC_IF((m_mask + 1) > max() / Grwoth_Factor,
            "the count reaches its limit, growth failed");
        return (m_mask + 1) * Grwoth_Factor;
    }

    size_t max() const noexcept {
        // 0xffffffff / 2 == 0x7fffffff
        // 0x7fffffff + 1 == 0x10000000
        return (std::numeric_limits<size_t>::max() / 2) + 1;
    }

    void clear() noexcept { m_mask = 0u; }

    size_t next_idx(size_t idx) noexcept { return (idx + 1) & m_mask; }

private:
private:
    size_t m_mask;
};

static_assert(growth_policy<power_of_two_growth<2>>, "");

}  // namespace detail
}  // namespace container
}  // namespace coust
