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

Area::Area(Area&& other) noexcept {
    std::swap(m_begin, other.m_begin);
    std::swap(m_end, other.m_end);
    std::swap(m_scoped, other.m_scoped);
}

Area& Area::operator=(Area&& other) noexcept {
    std::swap(m_begin, other.m_begin);
    std::swap(m_end, other.m_end);
    std::swap(m_scoped, other.m_scoped);
    return *this;
}

Area::~Area() noexcept {
    if (!m_scoped) {
        aligned_free(m_begin);
    }
}

std::pair<void*, void*> Area::steal() noexcept {
    void* ret_begin = nullptr;
    void* ret_end = nullptr;
    std::swap(m_begin, ret_begin);
    std::swap(m_end, ret_end);
    m_scoped = true;
    return {ret_begin, ret_end};
}

void* Area::begin() const noexcept {
    return m_begin;
}

void* Area::end() const noexcept {
    return m_end;
}

size_t Area::size() const noexcept {
    return ptr_math::sub(m_end, m_begin);
}

bool Area::contained(void* p) const noexcept {
    return (p >= m_begin) && (p < m_end);
}

bool Area::is_scope() const noexcept {
    return m_scoped;
}

std::vector<Area> Area::split_areas(
    Area const& base_area, size_t sub_area_size) noexcept {
    size_t const sub_area_count = base_area.size() / sub_area_size;
    void* const sub_area_begin_begin = base_area.begin();
    std::vector<Area> ret{};
    ret.reserve(sub_area_count);

    for (void* sub_area_begin = sub_area_begin_begin;
         sub_area_begin != base_area.end();) {
        void* const sub_area_end = std::min(
            ptr_math::add(sub_area_begin, sub_area_size), base_area.end());
        ret.push_back(Area{sub_area_begin, sub_area_end});
        sub_area_begin = sub_area_end;
    }
    return ret;
}

}  // namespace memory
}  // namespace coust
