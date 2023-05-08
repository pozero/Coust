#pragma once

#include "utils/allocators/Allocator.h"

namespace coust {
namespace memory {

class HeapAllocator {
public:
    using stateful = std::false_type;

public:
    HeapAllocator() noexcept = default;
    HeapAllocator(HeapAllocator&&) noexcept = default;
    HeapAllocator(HeapAllocator const&) noexcept = default;
    HeapAllocator& operator=(HeapAllocator&&) noexcept = default;
    HeapAllocator& operator=(HeapAllocator const&) noexcept = default;

    void* allocate(size_t size, size_t alignment) noexcept;

    void deallocate(void* p, size_t) noexcept;
};

static_assert(detail::Allocator<HeapAllocator>, "");

}  // namespace memory
}  // namespace coust
