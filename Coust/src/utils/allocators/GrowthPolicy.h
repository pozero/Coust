#pragma once

#include "utils/Compiler.h"
#include "utils/allocators/Area.h"
#include "utils/allocators/MemoryPool.h"
#include "utils/Assert.h"
#include "utils/Log.h"
#include "utils/TypeName.h"

#include <deque>
#include <utility>
#include <type_traits>

namespace coust {
namespace memory {
namespace detail {

struct GP_Attached {
    MemoryPool& m_pool;
};

struct GP_Scope {
    std::deque<Area>::const_iterator m_next_free_area;
};

}  // namespace detail

// 1) attached to a memory pool
// 2) attached to a block of memory (on stack or heap)
// 3) using aligned_alloc / aligend_free
enum class GrowthType {
    attached,
    scope,
    heap,
};

std::string_view constexpr to_string_view(GrowthType t) {
    switch (t) {
        case GrowthType::attached:
            return "attached";
        case GrowthType::scope:
            return "scope";
        case GrowthType::heap:
            return "heap";
    }
    ASSUME(0);
}

template <GrowthType Type, Size Growth_Factor>
class GrowthPolicy : public std::conditional_t<Type == GrowthType::attached,
                         detail::GP_Attached,
                         std::conditional_t<Type == GrowthType::scope,
                             detail::GP_Scope, detail::Empty>> {
public:
    using Base =
        std::conditional_t<Type == GrowthType::attached, detail::GP_Attached,
            std::conditional_t<Type == GrowthType::scope, detail::GP_Scope,
                detail::Empty>>;

public:
    GrowthPolicy(GrowthPolicy&&) = delete;
    GrowthPolicy(const GrowthPolicy&) = delete;
    GrowthPolicy& operator=(GrowthPolicy&&) = delete;
    GrowthPolicy& operator=(const GrowthPolicy&) = delete;

public:
    GrowthPolicy(MemoryPool& pool) noexcept
        requires(Type == GrowthType::attached)
        : Base(pool) {}

    GrowthPolicy(void* begin, void* end) noexcept
        requires(Type == GrowthType::scope)
    {
        split_area(begin, end);
        Base::m_next_free_area = m_areas.begin();
    }

    template <size_t Capacity>
    GrowthPolicy(std::array<char, Capacity>& stack_area) noexcept
        requires(Type == GrowthType::scope)
    {
        split_area((void*) stack_area.data(),
            (void*) ptr_math::add(stack_area.data(), Capacity));
        Base::m_next_free_area = m_areas.begin();
    }

    GrowthPolicy() noexcept
        requires(Type == GrowthType::heap)
    {}

    ~GrowthPolicy() noexcept {
        if constexpr (Type == GrowthType::attached) {
            for (auto& area : m_areas) {
                Base::m_pool.deallocate_area(std::move(area));
            }
        }
    }

    std::pair<void*, size_t> do_growth(size_t alignment) noexcept {
        if constexpr (Type == GrowthType::attached) {
            auto const& free_area = m_areas.emplace_front(
                Base::m_pool.allocate_area(Growth_Factor, alignment));
            // the size of area returned by memory pool might be bigger than the
            // growth factor, we return the actual size here
            return {free_area.begin(), free_area.size()};
        }
        if constexpr (Type == GrowthType::scope) {
            COUST_PANIC_IF(Base::m_next_free_area == m_areas.end(),
                "Growth policy failed: there isn't enough space to grow");
            auto const& free_area = *Base::m_next_free_area;
            Base::m_next_free_area++;
            return {free_area.begin(), free_area.size()};
        }
        if constexpr (Type == GrowthType::heap) {
            auto const& iter = m_areas.emplace_front(Growth_Factor, alignment);
            return {iter.begin(), iter.size()};
        }
    }

    bool contained(void* p) const noexcept
        requires(Type == GrowthType::attached || Type == GrowthType::scope)
    {
        return std::ranges::any_of(
            m_areas, [p](auto const& area) { return area.contained(p); });
    }

private:
    void split_area(void* begin, void* end) noexcept
        requires(Type == GrowthType::scope)
    {
        for (void *cur_area_begin = begin,
                  *cur_area_end = ptr_math::add(cur_area_begin, Growth_Factor);
             cur_area_begin < end; cur_area_begin = cur_area_end,
                  cur_area_end = std::min(
                      ptr_math::add(cur_area_end, Growth_Factor), end)) {
            m_areas.push_back(Area{cur_area_begin, cur_area_end});
        }
    }

private:
    std::deque<Area> m_areas;
};

// GrowableAllocator isn't "growable" according to the
// `detail::GrowableAllocator` concept. Hilarious, isn't it? :)
template <GrowthType Type, Size Growth_Factor,
    detail::GrowableAllocator Raw_Alloc>
class GrowableAllocator {
public:
    GrowableAllocator(GrowableAllocator&&) = delete;
    GrowableAllocator(const GrowableAllocator&) = delete;
    GrowableAllocator& operator=(GrowableAllocator&&) = delete;
    GrowableAllocator& operator=(const GrowableAllocator&) = delete;

public:
    using stateful = Raw_Alloc::stateful;

public:
    template <typename... Alloc_Args>
    GrowableAllocator(MemoryPool& pool, Alloc_Args&&... args) noexcept
        requires(Type == GrowthType::attached &&
                    std::constructible_from<Raw_Alloc, Alloc_Args...>)
        : m_growth_policy(pool),
          m_raw_allocator(std::forward<Alloc_Args>(args)...) {}

    template <typename... Alloc_Args>
    GrowableAllocator(void* begin, void* end, Alloc_Args&&... args) noexcept
        requires(Type == GrowthType::scope &&
                    std::constructible_from<Raw_Alloc, Alloc_Args...>)
        : m_growth_policy(begin, end),
          m_raw_allocator(std::forward<Alloc_Args>(args)...) {}

    template <size_t Capacity, typename... Alloc_Args>
    GrowableAllocator(
        std::array<char, Capacity>& stack_area, Alloc_Args&&... args) noexcept
        requires(Type == GrowthType::scope &&
                    std::constructible_from<Raw_Alloc, Alloc_Args...>)
        : m_growth_policy(stack_area),
          m_raw_allocator(std::forward<Alloc_Args>(args)...) {}

    GrowableAllocator() noexcept
        requires(Type == GrowthType::heap &&
                    std::is_default_constructible_v<Raw_Alloc>)
        : m_growth_policy(), m_raw_allocator() {}

    template <typename... Alloc_Args>
    GrowableAllocator(Alloc_Args&&... args) noexcept
        requires(Type == GrowthType::heap &&
                    std::constructible_from<Raw_Alloc, Alloc_Args...>)
        : m_growth_policy(),
          m_raw_allocator(std::forward<Alloc_Args>(args)...) {}

    void* allocate(size_t size, size_t alignment = DEFAULT_ALIGNMENT) noexcept {
        void* ret_ptr = m_raw_allocator.allocate(size, alignment);
        if (!ret_ptr) {
            auto const [new_area_ptr, new_area_size] =
                m_growth_policy.do_growth(alignment);
            m_raw_allocator.grow(new_area_ptr, new_area_size);
            ret_ptr = m_raw_allocator.allocate(size, alignment);
        }
        return ret_ptr;
    }

    void deallocate(void* p, size_t size) noexcept {
        if constexpr (Type == GrowthType::attached ||
                      Type == GrowthType::scope) {
            COUST_PANIC_IF_NOT(m_growth_policy.contained(p),
                "The instance of GrowableAllocator<{}, {}, {}> does not "
                "contain the memory block: ptr {}, size {}",
                to_string_view(Type), to_string_view(Growth_Factor),
                type_name<Raw_Alloc>(), p, size);
            m_raw_allocator.deallocate(p, size);
        } else
            m_raw_allocator.deallocate(p, size);
    }

    Raw_Alloc const& get_raw_allocator() const noexcept {
        return m_raw_allocator;
    }

    Raw_Alloc& get_raw_allocator() noexcept { return m_raw_allocator; }

private:
    GrowthPolicy<Type, Growth_Factor> m_growth_policy;
    Raw_Alloc m_raw_allocator;
};

}  // namespace memory
}  // namespace coust
