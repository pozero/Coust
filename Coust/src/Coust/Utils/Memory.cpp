#include "pch.h"

#include "Coust/Utils/Memory.h"

namespace Coust::Memory 
{
    template <>
    Area<AreaType::Static>::Area(size_t size, size_t alignment) noexcept
    {
        COUST_CORE_ASSERT(size > 0, "Allocating an area with 0 size isn't well defined");
        m_Begin = AlignedAlloc(size, alignment);
        m_End = PointerMath::Add(m_Begin, size);
    }

    template <>
    Area<AreaType::Static>::Area(void* begin, void* end) noexcept
        : m_Begin(begin), m_End(end)
    {
    }

    template <>
    Area<AreaType::Static>::~Area() noexcept
    {
        AlignedFree(m_Begin);
    }

    template <>
    Area<AreaType::Scope>::~Area() noexcept
    {
    }

    StackAllocator::StackAllocator(void* begin, void* end) noexcept
        : m_Begin(begin), m_End(end), m_Top(begin)
    {}

    void* StackAllocator::Allocate(size_t size, size_t alignment) noexcept
    {
        COUST_CORE_ASSERT(alignment <= ALIGNMENT, 
            "The alignment requirement {} exceeds the fixed alignment {} of stack allocator", alignment, ALIGNMENT);
        const size_t roundedUpSize = PointerMath::RoundUpToAligned(size, ALIGNMENT);
        void* const begin = m_Top;
        void* const end = PointerMath::Add(m_Top, roundedUpSize);
        if (end <= m_End) [[likely]]
        {
            m_Top = end;
            return begin;
        }
        return nullptr;
    }

    void StackAllocator::Free(void* p, size_t size) noexcept
    {
        const size_t roundedUpSize = PointerMath::RoundUpToAligned(size, ALIGNMENT);
        void* const end = PointerMath::Add(p, roundedUpSize);
        COUST_CORE_PANIC_IF(end != m_Top, "Stack allocator free operation out of order!");
        m_Top = p;
    }

    void StackAllocator::Reset() noexcept
    {
        m_Top = m_Begin;
    }

    void* HeapAllocator::Allocate(size_t size, size_t alignment)
    {
        return AlignedAlloc(size, alignment);
    }

    void HeapAllocator::Free(void* p, size_t) noexcept
    {
        AlignedFree(p);
    }

    void NoLock::lock() noexcept 
    {
    }

    void NoLock::unlock() noexcept 
    {
    }
}