#pragma once

#include "utils/Compiler.h"

namespace coust {
namespace memory {

class Area {
public:
    Area(Area const&) = delete;
    Area& operator=(Area const&) = delete;

public:
    Area() noexcept = default;

    // the area owns its memory block
    Area(size_t size, size_t alignment) noexcept;

    // the area is a scope, it doesn't own this memory block
    Area(void* begin, void* end) noexcept;

    Area(Area&& other) noexcept;

    Area& operator=(Area&& other) noexcept;

    ~Area() noexcept;

    std::pair<void*, void*> steal() noexcept;

    void* begin() const noexcept;

    void* end() const noexcept;

    size_t size() const noexcept;

    bool contained(void* p) const noexcept;

    bool is_scope() const noexcept;

private:
    void* m_begin = nullptr;
    void* m_end = nullptr;
    bool m_scoped = true;
};

}  // namespace memory
}  // namespace coust
