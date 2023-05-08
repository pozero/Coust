#pragma once

#include "utils/allocators/Allocator.h"
#include "utils/allocators/Area.h"

namespace coust {
namespace memory {

class MonotonicAllocator {
public:
    MonotonicAllocator(MonotonicAllocator&&) = delete;
    MonotonicAllocator(const MonotonicAllocator&) = delete;
    MonotonicAllocator& operator=(MonotonicAllocator&&) = delete;
    MonotonicAllocator& operator=(const MonotonicAllocator&) = delete;

public:
    using stateful = std::true_type;

public:
    MonotonicAllocator() noexcept = default;

    MonotonicAllocator(void* begin, void* end) noexcept;

    void* allocate(size_t size, size_t alignment) noexcept;

    void deallocate(void*, size_t) noexcept;

    void grow(void* p, size_t size) noexcept;

private:
    void* m_cur_begin = nullptr;
    void* m_cur_end = nullptr;
};

static_assert(detail::GrowableAllocator<MonotonicAllocator>, "");

}  // namespace memory
}  // namespace coust
