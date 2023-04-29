#include "utils/allocators/Allocator.h"

namespace coust {
namespace memory {

class FreeListAllocator {
public:
    FreeListAllocator() = delete;
    FreeListAllocator(FreeListAllocator const&) = delete;
    FreeListAllocator& operator=(FreeListAllocator const&) = delete;

public:
    void* allocate(size_t size, size_t alignment) noexcept;

    void free(void* p, size_t size) noexcept;

private:
};

static_assert(detail::Allocator<FreeListAllocator>, "");

}  // namespace memory
}  // namespace coust
