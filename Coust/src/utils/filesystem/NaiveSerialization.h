#pragma once

#include "utils/TypeName.h"
#include "utils/Compiler.h"
#include "utils/Assert.h"
#include "utils/PtrMath.h"
#include "utils/filesystem/FileIO.h"

namespace coust {
namespace file {
namespace detail {

struct any {
    template <typename T>
    operator T();
};

// https://youtu.be/myhB8ZlwOlE?t=568
template <typename T, typename... Args>
consteval auto member_count() {
    bool constexpr is_aggreagte = std::is_aggregate_v<std::remove_cvref_t<T>>;
    bool constexpr provided_count = requires { T::member_count::value; };
    if constexpr (provided_count) {
        return T::member_count::value;
    } else if constexpr (is_aggreagte) {
        // for aggregate type, its initialization list can take no more than
        // member_count variables
        if constexpr (requires { T{{Args{}}..., {any{}}}; } == false) {
            return sizeof...(Args);
        } else {
            return member_count<T, Args..., any>();
        }
    }
}

FORCE_INLINE void visit_members(auto&& object, auto&& func) noexcept {
    auto constexpr count =
        member_count<std::remove_cvref_t<decltype(object)>>();
    static_assert(count <= 20);
    if constexpr (count == 0) {
        return func();
    } else if constexpr (count == 1) {
        auto& [a1] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        func(a1);
    } else if constexpr (count == 2) {
        auto& [a1, a2] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        func(a1, a2);
    } else if constexpr (count == 3) {
        auto& [a1, a2, a3] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        func(a1, a2, a3);
    } else if constexpr (count == 4) {
        auto& [a1, a2, a3, a4] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        func(a1, a2, a3, a4);
    } else if constexpr (count == 5) {
        auto& [a1, a2, a3, a4, a5] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        func(a1, a2, a3, a4, a5);
    } else if constexpr (count == 6) {
        auto& [a1, a2, a3, a4, a5, a6] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        func(a1, a2, a3, a4, a5, a6);
    } else if constexpr (count == 7) {
        auto& [a1, a2, a3, a4, a5, a6, a7] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        func(a1, a2, a3, a4, a5, a6, a7);
    } else if constexpr (count == 8) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8);
    } else if constexpr (count == 9) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9);
    } else if constexpr (count == 10) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10);
    } else if constexpr (count == 11) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11);
    } else if constexpr (count == 12) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12);
    } else if constexpr (count == 13) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13);
    } else if constexpr (count == 14) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14] =
            object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14);
    } else if constexpr (count == 15) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14,
            a15] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        static_assert(!std::is_pointer_v<decltype(a15)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15);
    } else if constexpr (count == 16) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        static_assert(!std::is_pointer_v<decltype(a15)>);
        static_assert(!std::is_pointer_v<decltype(a16)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16);
    } else if constexpr (count == 17) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        static_assert(!std::is_pointer_v<decltype(a15)>);
        static_assert(!std::is_pointer_v<decltype(a16)>);
        static_assert(!std::is_pointer_v<decltype(a17)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17);
    } else if constexpr (count == 18) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        static_assert(!std::is_pointer_v<decltype(a15)>);
        static_assert(!std::is_pointer_v<decltype(a16)>);
        static_assert(!std::is_pointer_v<decltype(a17)>);
        static_assert(!std::is_pointer_v<decltype(a18)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18);
    } else if constexpr (count == 19) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        static_assert(!std::is_pointer_v<decltype(a15)>);
        static_assert(!std::is_pointer_v<decltype(a16)>);
        static_assert(!std::is_pointer_v<decltype(a17)>);
        static_assert(!std::is_pointer_v<decltype(a18)>);
        static_assert(!std::is_pointer_v<decltype(a19)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19);
    } else if constexpr (count == 20) {
        auto& [a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20] = object;
        static_assert(!std::is_pointer_v<decltype(a1)>);
        static_assert(!std::is_pointer_v<decltype(a2)>);
        static_assert(!std::is_pointer_v<decltype(a3)>);
        static_assert(!std::is_pointer_v<decltype(a4)>);
        static_assert(!std::is_pointer_v<decltype(a5)>);
        static_assert(!std::is_pointer_v<decltype(a6)>);
        static_assert(!std::is_pointer_v<decltype(a7)>);
        static_assert(!std::is_pointer_v<decltype(a8)>);
        static_assert(!std::is_pointer_v<decltype(a9)>);
        static_assert(!std::is_pointer_v<decltype(a10)>);
        static_assert(!std::is_pointer_v<decltype(a11)>);
        static_assert(!std::is_pointer_v<decltype(a12)>);
        static_assert(!std::is_pointer_v<decltype(a13)>);
        static_assert(!std::is_pointer_v<decltype(a14)>);
        static_assert(!std::is_pointer_v<decltype(a15)>);
        static_assert(!std::is_pointer_v<decltype(a16)>);
        static_assert(!std::is_pointer_v<decltype(a17)>);
        static_assert(!std::is_pointer_v<decltype(a18)>);
        static_assert(!std::is_pointer_v<decltype(a19)>);
        static_assert(!std::is_pointer_v<decltype(a20)>);
        func(a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15,
            a16, a17, a18, a19, a20);
    } else {
        static_assert(std::is_void_v<decltype(object)>, "");
    }
    // Look at these beautiful codes, that's why we all love c++!
    // also thanks vim :D
}

// https://youtu.be/G7-GQhCw8eE
// naive serialization implmentation, only work for:
// 1) fundamental type & enum
// 2) trivial type with all public access of all members
// 3) containers satisying `std::ranges::contiguous_range` concept
//    (https://en.cppreference.com/w/cpp/ranges/contiguous_range)
// 4) type that implements the method `serialize(Archive &) const noexcept`

struct ArchiveIn {};
struct ArchiveOut {};

template <typename Kind>
class Archive {
public:
    Archive() = delete;

    Archive(Archive&&) noexcept = default;
    Archive(Archive const&) noexcept = default;
    Archive& operator=(Archive&&) noexcept = default;
    Archive& operator=(Archive const&) noexcept = default;

public:
    Archive(ByteArray& data, Kind) : m_data(data) {}

    size_t poisition() const noexcept { return m_pos; }

    FORCE_INLINE void operator()(auto&&... objects) noexcept {
        serialize_many(objects...);
    }

    FORCE_INLINE void serialize_many(auto&&... objects) noexcept {
        (serialize_one(objects), ...);
    }

    FORCE_INLINE void serialize_many() noexcept {}

    FORCE_INLINE void serialize_one(auto&& object) noexcept {
        using type = std::remove_cvref_t<decltype(object)>;
        bool constexpr provided_own_implementation =
            requires(type& t, Archive& archive) {
                { type::serialize(t, archive) } noexcept;
            };
        bool constexpr is_simple =
            !provided_own_implementation &&
            (std::is_fundamental_v<type> || std::is_enum_v<type>);
        bool constexpr is_contiguous_container =
            !provided_own_implementation && std::ranges::contiguous_range<type>;
        bool constexpr is_trivially_accessible =
            // std::array would be ragraded as trivially accessible...
            (!is_contiguous_container && !provided_own_implementation) &&
            requires {
                member_count<type>() > 0;
                visit_members(std::declval<type>(), [](auto&&...) {});
            };

        static_assert(provided_own_implementation || is_simple ||
                      is_trivially_accessible || is_contiguous_container);

        if constexpr (provided_own_implementation) {
            type::serialize(object, *this);
        } else if constexpr (is_simple) {
            serialize_bytes_of(object);
        }
        // the byte layout of containers looks like this:
        // +-------+------------------+
        // | size  |       ...        |
        // +-------+------------------+
        else if constexpr (is_contiguous_container) {
            using contained_type = std::ranges::range_value_t<type>;
            auto count = std::ranges::size(object);
            serialize_bytes_of(count);
            if constexpr (std::is_same_v<Kind, ArchiveIn> &&
                          requires(type& t) { t.resize(size_t{}); }) {
                object.resize(count);
            }

            if constexpr (std::is_trivially_copyable_v<contained_type> &&
                          requires {
                              std::declval<Archive<ArchiveIn>>().serialize_one(
                                  std::declval<contained_type>());
                              std::declval<Archive<ArchiveOut>>().serialize_one(
                                  std::declval<contained_type>());
                          }) {
                // if the container is contiguous, there's no need to care about
                // padding
                auto const bytes_count = count * sizeof(contained_type);
                auto const contiguous_data_begin = std::ranges::data(object);
                serialize_range_of_bytes(contiguous_data_begin, bytes_count);
            } else {
                for (auto& contained : object) {
                    serialize_one(contained);
                }
            }
        } else if constexpr (is_trivially_accessible) {
            visit_members(object, [this](auto&&... members) FORCE_INLINE {
                (this->serialize_one(members), ...);
            });
        }
    }

    FORCE_INLINE void serialize_bytes_of(auto&& object) noexcept {
        using type = std::remove_cvref_t<decltype(object)>;
        size_t constexpr proceed_size = sizeof(type);

        if constexpr (std::is_same_v<Kind, ArchiveOut>) {
            if (proceed_size > m_data.size() - m_pos) {
                size_t const new_size = std::max(
                    (size_t) (float(m_data.size()) * BUFFER_GROWTH_FACTOR),
                    proceed_size + m_pos);
                m_data.grow_to(new_size);
            }
            auto const cur_pos = ptr_math::add(m_data.data(), m_pos);
            memcpy(cur_pos, &object, proceed_size);
        } else if constexpr (std::is_same_v<Kind, ArchiveIn>) {
            auto const cur_pos = ptr_math::add(m_data.data(), m_pos);
            memcpy(&object, cur_pos, proceed_size);
        }
        m_pos += proceed_size;
    }

    FORCE_INLINE void serialize_range_of_bytes(auto ptr, auto size) noexcept {
        size_t const proceed_size = size;

        if constexpr (std::is_same_v<Kind, ArchiveOut>) {
            if (proceed_size > m_data.size() - m_pos) {
                size_t const new_size = std::max(
                    (size_t) (float(m_data.size()) * BUFFER_GROWTH_FACTOR),
                    proceed_size + m_pos);
                m_data.grow_to(new_size);
            }
            auto const cur_pos = ptr_math::add(m_data.data(), m_pos);
            memcpy(cur_pos, ptr, proceed_size);
        } else if constexpr (std::is_same_v<Kind, ArchiveIn>) {
            auto const cur_pos = ptr_math::add(m_data.data(), m_pos);
            memcpy(ptr, cur_pos, proceed_size);
        }
        m_pos += proceed_size;
    }

private:
    static float constexpr BUFFER_GROWTH_FACTOR = 2.0f;

private:
    ByteArray& m_data;
    size_t m_pos = 0u;
};

template <typename T>
Archive(ByteArray&, T) -> Archive<T>;

}  // namespace detail

template <typename T>
FORCE_INLINE ByteArray to_byte_array(T&& object)
    requires(std::is_default_constructible_v<std::remove_cvref_t<T>>)
{
    using type = std::remove_cvref_t<T>;
    if constexpr (std::ranges::contiguous_range<type>) {
        size_t const size = std::ranges::size(object) *
                            sizeof(std::ranges::range_value_t<type>);
        ByteArray ret{size, alignof(T)};
        detail::Archive archive{ret, detail::ArchiveOut{}};
        archive(std::forward<T>(object));
        return ret;
    } else {
        ByteArray ret{sizeof(T), alignof(T)};
        detail::Archive archive{ret, detail::ArchiveOut{}};
        if constexpr (std::is_fundamental_v<type> || std::is_enum_v<type>) {
            archive(object);
        } else {
            archive(std::forward<T>(object));
        }
        return ret;
    }
}

template <typename T>
FORCE_INLINE T from_byte_array(ByteArray& byte_array)
    requires(std::is_default_constructible_v<std::remove_cvref_t<T>>)
{
    T ret{};
    detail::Archive archive{byte_array, detail::ArchiveIn{}};
    archive(ret);
    return ret;
}

template <typename T>
FORCE_INLINE void from_byte_array(ByteArray& byte_array, T& out_obj) {
    detail::Archive archive{byte_array, detail::ArchiveIn{}};
    archive(out_obj);
}

}  // namespace file
}  // namespace coust
