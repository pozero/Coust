#include "utils/allocators/Allocator.h"

namespace coust {
namespace memory {

class StackAllocator {
public:
    StackAllocator() = delete;
    StackAllocator(StackAllocator const&) = delete;
    StackAllocator& operator=(StackAllocator const&) = delete;

public:
    void* allocate(size_t size, size_t alignment) noexcept;

    void free(void* p, size_t size) noexcept;
};

static_assert(detail::Allocator<StackAllocator>, "");

}  // namespace memory
}  // namespace coust
