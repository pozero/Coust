#pragma once

#include "utils/allocators/Allocator.h"
#include "utils/allocators/Area.h"

namespace coust {
namespace memory {

class FreeListAllocator {
public:
    FreeListAllocator(FreeListAllocator&&) = delete;
    FreeListAllocator(FreeListAllocator const&) = delete;
    FreeListAllocator& operator=(FreeListAllocator&&) = delete;
    FreeListAllocator& operator=(FreeListAllocator const&) = delete;

public:
    using stateful = std::true_type;

public:
    FreeListAllocator() noexcept = default;

    FreeListAllocator(void* begin, void* end) noexcept;

    void* allocate(size_t size, size_t alignment) noexcept;

    void deallocate(void* p, size_t) noexcept;

    void grow(void* p, size_t size) noexcept;

private:
    // layout of the header of memory block in use:
    //                                                   pointer allocated
    //                                                          |
    //                                                          |
    //                                                          v
    // +---------------+------------+-----------+---------------+
    // | uint32_t size | void* next | [padding] | uint32_t head |
    // +---------------+------------+-----------+---------------+
    // |<----------------------- head ------------------------->|
    //
    // layout of the header of free memory block:
    // +---------------+------------+
    // | uint32_t size | void* next |
    // +---------------+------------+
    struct BlockHeader {
        // the size of the **entire** memory block. `uint32_t` can represent up
        // to 4 GiB memory size. that should be enough.
        uint32_t size = 0;
        BlockHeader* next = nullptr;
    };

private:
    BlockHeader* m_first_free_block = nullptr;

// the logic is rather complicated and needs its own private member and method
// to record and test.
#if defined(COUST_TEST)
private:
    size_t m_free_block_count;
    size_t m_free_block_size;

public:
    void is_malfunctioning() const noexcept;
#endif
};

static_assert(detail::GrowableAllocator<FreeListAllocator>, "");

}  // namespace memory
}  // namespace coust
