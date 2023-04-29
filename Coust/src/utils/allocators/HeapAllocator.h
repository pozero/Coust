#include "utils/allocators/Allocator.h"

namespace coust {
namespace memory {

class HeapAllocator {
public:
    HeapAllocator() = delete;
    HeapAllocator(HeapAllocator const&) = delete;
    HeapAllocator& operator=(HeapAllocator const&) = delete;

public:
    void* allocate(size_t size, size_t alignment) noexcept;

    void free(void* p, size_t size) noexcept;
};

static_assert(detail::Allocator<HeapAllocator>, "");

}  // namespace memory
}  // namespace coust
