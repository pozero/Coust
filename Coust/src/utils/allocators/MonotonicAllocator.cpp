#include "pch.h"

#include "utils/Compiler.h"
#include "utils/allocators/MonotonicAllocator.h"

namespace coust {
namespace memory {

MonotonicAllocator::MonotonicAllocator(void* begin, void* end) noexcept
    : m_cur_begin(begin), m_cur_end(end) {
}

void* MonotonicAllocator::allocate(size_t size, size_t alignment) noexcept {
    void* const ret_ptr = ptr_math::align(m_cur_begin, alignment);
    void* const next_begin = ptr_math::add(ret_ptr, size);
    if (next_begin > m_cur_end)
        return nullptr;
    m_cur_begin = next_begin;
    return ret_ptr;
}

void MonotonicAllocator::deallocate(
    [[maybe_unused]] void*, [[maybe_unused]] size_t) noexcept {
}

void MonotonicAllocator::grow(void* p, size_t size) noexcept {
    m_cur_begin = p;
    m_cur_end = ptr_math::add(p, size);
}

}  // namespace memory
}  // namespace coust
