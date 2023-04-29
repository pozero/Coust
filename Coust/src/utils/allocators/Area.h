#pragma once

namespace coust {
namespace memory {

class Area {
public:
    Area() = delete;
    Area(Area const&) = delete;
    Area& operator=(Area const&) = delete;

public:
    // the area owns its memory block
    Area(size_t size, size_t alignment) noexcept;

    // the area is a scope, it doesn't own this memory block
    Area(void* begin, void* end) noexcept;

    ~Area() noexcept;

    void* begin() const noexcept;

    void* end() const noexcept;

    bool contained(void* p) const noexcept;

    bool is_scope() const noexcept;

private:
    void* const m_begin = nullptr;
    void* const m_end = nullptr;
    bool const m_scoped;
};

}  // namespace memory
}  // namespace coust
