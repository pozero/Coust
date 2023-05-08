#include "pch.h"

#include "utils/Compiler.h"
#include "utils/allocators/HeapAllocator.h"

namespace coust {
namespace memory {

void* HeapAllocator::allocate(size_t size, size_t alignment) noexcept {
    return aligned_alloc(size, alignment);
}

void HeapAllocator::deallocate(void* p, [[maybe_unused]] size_t) noexcept {
    aligned_free(p);
}

}  // namespace memory
}  // namespace coust
