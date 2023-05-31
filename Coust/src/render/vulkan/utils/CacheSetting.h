#pragma once

namespace coust {
namespace render {

uint32_t constexpr GARBAGE_COLLECTION_PERIOD = 10;

class GCTimer {
public:
    explicit GCTimer(uint32_t gc_period) noexcept;

    void tick() noexcept;

    bool should_recycle(uint32_t last_accessed) const noexcept;

    uint32_t current_count() const noexcept;

private:
    uint32_t const m_gc_period;
    uint32_t m_count = 0;
};

class CacheHitCounter {
public:
    CacheHitCounter(std::string_view name) noexcept;

    ~CacheHitCounter() noexcept;

    void hit() noexcept;

    void miss() noexcept;

private:
    std::string_view m_name;
    uint64_t m_hit_cnt = 0;
    uint64_t m_miss_cnt = 0;
};

}  // namespace render
}  // namespace coust
