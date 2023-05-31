#include "pch.h"

#include "utils/Log.h"
#include "render/vulkan/utils/CacheSetting.h"

namespace coust {
namespace render {

GCTimer::GCTimer(uint32_t gc_period) noexcept
    : m_gc_period(gc_period), m_count(gc_period) {
}

void GCTimer::tick() noexcept {
    ++m_count;
}

bool GCTimer::should_recycle(uint32_t last_accessed) const noexcept {
    return (m_count - m_gc_period) > last_accessed;
}

uint32_t GCTimer::current_count() const noexcept {
    return m_count;
}

CacheHitCounter::CacheHitCounter(std::string_view name) noexcept
    : m_name(name) {
}

CacheHitCounter::~CacheHitCounter() noexcept {
    if (m_hit_cnt + m_miss_cnt == 0)
        return;

    uint64_t hit_percentage = (uint64_t) (((float) m_hit_cnt * 100.0f) /
                                          (float) (m_hit_cnt + m_miss_cnt));
    COUST_INFO("{} Hit Percentage: {}%", m_name, hit_percentage);
}

void CacheHitCounter::hit() noexcept {
    ++m_hit_cnt;
}

void CacheHitCounter::miss() noexcept {
    ++m_miss_cnt;
}

}  // namespace render
}  // namespace coust
