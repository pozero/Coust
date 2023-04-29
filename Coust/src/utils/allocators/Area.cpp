#include "pch.h"

#include "utils/allocators/Allocator.h"
#include "utils/allocators/Area.h"

namespace coust {
namespace memory {

Area::Area(size_t size, size_t alignment) noexcept
    : m_begin(aligned_alloc(size, alignment)),
      m_end(ptr_math::add(m_begin, size)),
      m_scoped(false) {
}

Area::Area(void* begin, void* end) noexcept
    : m_begin(begin), m_end(end), m_scoped(true) {
}

Area::~Area() noexcept {
    if (!m_scoped) {
        aligned_free(m_begin);
    }
}

void* Area::begin() const noexcept {
    return m_begin;
}

void* Area::end() const noexcept {
    return m_end;
}

bool Area::contained(void* p) const noexcept {
    return (p >= m_begin) && (p < m_end);
}

bool Area::is_scope() const noexcept {
    return m_scoped;
}

}  // namespace memory
}  // namespace coust
