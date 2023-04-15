#pragma once

#include <utility>
#include <cstddef>
#include <string>
#include <sstream>
#include <type_traits>
#include <typeinfo>

#include "Coust/Core/Logger.h"
#include "Coust/Utils/FileSystem.h"

namespace Coust::Memory 
{
    namespace Detail 
    {
        template <typename E>
        constexpr bool Has(E lhs, E rhs) noexcept
            requires (std::is_enum_v<E> && std::is_arithmetic_v<std::underlying_type_t<E>>)
        {
            using N = std::underlying_type_t<E>;
            return ((N(lhs) & N(rhs)) != N(0));
        }

        template <typename... CLASS_TO_INHERIT>
        class Pack : public CLASS_TO_INHERIT... {};

        template <typename, template <typename, typename...> typename>
        struct IsInstance : public std::false_type {};

        template <typename... Ts,  template <typename, typename...> typename U>
        struct IsInstance<U<Ts...>, U> : public std::true_type {};
    }

    namespace PointerMath
    {
        template <typename P, typename N>
        inline P* Add(P* base, N bytes) noexcept
        {
            return (P*) (uintptr_t(base) + uintptr_t(bytes));
        }

        template <typename P, typename N>
        inline P* Sub(P* base, N bytes) noexcept
        {
            return (P*) (uintptr_t(base) - uintptr_t(bytes));
        }

        template <typename P>
        inline size_t Sub(P* l, P* r) noexcept
        {
            return (size_t) (uintptr_t(l) - uintptr_t(r));
        }

        template <typename P>
        inline P* Align(P* p, size_t alignment) noexcept
        {
            return (P*) ((uintptr_t(p) + alignment - 1) & (~(alignment - 1)));
        }

        inline size_t RoundUpToAligned(size_t size, size_t alignment) noexcept
        {
            return ((size - 1) & ~(alignment - 1)) + alignment;
        }
    }

    inline void* AlignedAlloc(size_t size, size_t alignment) noexcept
    {
        // https://en.cppreference.com/w/cpp/memory/c/aligned_alloc#Notes
        // As an example of the "supported by the implementation" requirement, 
        // POSIX function posix_memalign accepts any alignment that is a power of two and a multiple of sizeof(void*), 
        // and POSIX-based implementations of aligned_alloc inherit this requirements. 
        // Also, this function is not supported in Microsoft Visual C++ because 
        // its implementation of std::free() is unable to handle aligned allocations of any kind. 
        // Instead, MSVC provides _aligned_malloc (to be freed with _aligned_free).
        alignment = (alignment < sizeof(void*)) ? sizeof(void*) : alignment;
        void* p = nullptr;
    #ifdef _MSC_VER
        p = _aligned_malloc(size, alignment);
    #else
        p = std::aligned_alloc(size, alignment);
    #endif
        return p;
    }

    inline void AlignedFree(void* p) noexcept
    {
    #ifdef _MSC_VER
        _aligned_free(p);
    #else
        std::free(p);
    #endif
    }

    enum AreaSize : size_t
    {
        Kib = 1024,
        Mib = 1024 * 1024,
    };

    enum class AreaType
    {
        // The scope area is a memory view, it just wraps around a block of memory
        Scope,
        // The static area actually manages a block of memory
        Static
    };

    template <AreaType TYPE = AreaType::Static>
    class Area 
    {
    public:
        Area() = delete;
        Area(const Area&) = delete;
        Area& operator=(Area&&) = delete;
        Area& operator=(const Area&) = delete;

    public:
        Area(Area&& other) noexcept
        {
            std::swap(m_Begin, other.m_Begin);
            std::swap(m_End, other.m_End);
        }

        explicit Area(size_t size, size_t alignment = alignof(std::max_align_t)) noexcept;

        explicit Area(void* begin, void* end) noexcept;

        ~Area() noexcept;

        void* Begin() const noexcept { return m_Begin; }

        void* End() const noexcept { return m_End; }

        size_t Size() const noexcept { return PointerMath::Sub(m_End, m_Begin); }

        bool Inside(void* p) const noexcept
        {
            return p >= m_Begin && p <= m_End;
        }

    private:
        void* m_Begin = nullptr;
        void* m_End = nullptr;
    };

    enum class TrackType
    {
        None = 0,
        // Occupancy track can report the highest occupancy rate and overflow size
        Occupancy = 1,
        // Access track will set the memory to specific value when other codes
        // manipulating them, so we can tell what happened before some memory
        // bugs show up.
        Access = 2,
        // Track allocation behavior (only do so when required or we will get a HUGE log file)
        Allocation = 4,
        // Record all allocation size
        // Useful for set appropriate memory area size
        Size = 8,

        All = Occupancy | Access | Allocation | Size,
    };

    namespace Detail 
    {
        struct TrackParamBase
        {
            std::string_view name{};
            void* base = nullptr;
            size_t size = 0;
        };

        struct TrackParamOccupance
        {
            size_t currentOccupance = 0;
            size_t highestOccupance = 0;
            size_t currentOverflow = 0;
            size_t highestOverflow = 0;
        };

        struct TrackParamAllocation
        {
            std::unique_ptr<Coust::Logger> allocLogger;
        };

        struct TrackParamSize
        {
            std::unique_ptr<Coust::Logger> sizeLogger;
        };
    }

    template <TrackType TYPE>
    class Track : protected std::conditional_t<TYPE == TrackType::None, Detail::Pack<>, 
        std::conditional_t<Detail::Has(TYPE, TrackType::Occupancy), 
            std::conditional_t<Detail::Has(TYPE, TrackType::Allocation), 
                std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance, Detail::TrackParamAllocation, Detail::TrackParamSize>, 
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance, Detail::TrackParamAllocation>>,
                std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance, Detail::TrackParamSize>,
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance>>>, 
            std::conditional_t<Detail::Has(TYPE, TrackType::Allocation), 
                std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamAllocation, Detail::TrackParamSize>,
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamAllocation>>, 
                std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                    Detail::Pack<Detail::TrackParamBase, Detail::TrackParamSize>,
                    Detail::Pack<Detail::TrackParamBase>>>>>
    {
    public:
        using Base = std::conditional_t<TYPE == TrackType::None, Detail::Pack<>, 
            std::conditional_t<Detail::Has(TYPE, TrackType::Occupancy), 
                std::conditional_t<Detail::Has(TYPE, TrackType::Allocation), 
                    std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance, Detail::TrackParamAllocation, Detail::TrackParamSize>, 
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance, Detail::TrackParamAllocation>>,
                    std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance, Detail::TrackParamSize>,
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamOccupance>>>, 
                std::conditional_t<Detail::Has(TYPE, TrackType::Allocation), 
                    std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamAllocation, Detail::TrackParamSize>,
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamAllocation>>, 
                    std::conditional_t<Detail::Has(TYPE, TrackType::Size), 
                        Detail::Pack<Detail::TrackParamBase, Detail::TrackParamSize>,
                        Detail::Pack<Detail::TrackParamBase>>>>>;
    public:
        Track() = delete;
        Track(const Track &) = delete;
        Track &operator=(Track &&) = delete;
        Track &operator=(const Track &) = delete;
    
    public:
        Track(Track &&) noexcept = default;

        Track(std::string_view, void*, size_t) noexcept
            requires (TYPE == TrackType::None)
        {}

        Track(std::string_view name, void* base, size_t size) noexcept
            requires (TYPE != TrackType::None)
            : Base{ name, base, size }
        {
            if constexpr (Detail::Has(TYPE, TrackType::Allocation))
            {
                std::string fileName { Base::name };
                fileName += " Allocation.log";
                Base::allocLogger = std::make_unique<Logger>(fileName.c_str(), "%^%v%$", false);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Size))
            {
                std::string fileName { Base::name };
                fileName += " Size.log";
                Base::sizeLogger = std::make_unique<Logger>(fileName.c_str(), "%^%v%$", false);
            }
        }

        ~Track() noexcept
        {
            if constexpr (Detail::Has(TYPE, TrackType::Occupancy))
            {
                size_t percentage = (100 * Base::highestOccupance) / (Base::size);
                COUST_CORE_INFO("{} Arena Highest Occupancy: {} Kib {} Byte ({}%)", 
                    Base::name, Base::highestOccupance / 1024, Base::highestOccupance % 1024, percentage);
                
                if (Base::highestOverflow > 0)
                    COUST_CORE_INFO("{} Arena Oveflow: {} Kib {} Byte", 
                        Base::name, Base::highestOverflow / 1024, Base::highestOverflow % 1024);
            }
        }

        void OnAllocation(void* p, size_t size, size_t alignment,
                          const char* file, int line, const char* allocatorName, const char* typeName, const size_t count) noexcept
        {
            if constexpr (Detail::Has(TYPE, TrackType::Occupancy))
            {
                Base::currentOccupance += size;
                Base::highestOccupance = std::max(Base::currentOccupance, Base::highestOccupance);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Access))
            {
                memset(p, 'A', size);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Allocation))
            {
                if (file)
                    Base::allocLogger->Get().trace("[{}:{}] ({}) Alloc {} of {} / {} bytes -> {:>}", file, line, allocatorName, count, typeName, size, p);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Size))
            {
                Base::sizeLogger->Get().trace(size);
            }
        }

        void OnFree(void* p, size_t size,
                    const char* file, int line, const char* allocatorName, const char* typeName) noexcept
        {
            if constexpr (Detail::Has(TYPE, TrackType::Occupancy))
            {
                Base::currentOccupance -= size;
            }

            if constexpr (Detail::Has(TYPE, TrackType::Access))
            {
                memset(p, 'F', size);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Allocation))
            {
                if (file)
                    Base::allocLogger->Get().trace("[{}:{}] ({}) Free {} / {} bytes -> {:>}", file, line, allocatorName, typeName, size, p);
            }
        }

        void OnOverflowAlloc(void* p, size_t size, size_t alignment,
                             const char* file, int line, const char* allocatorName, const char* typeName, const size_t count) noexcept
        {
            if constexpr (Detail::Has(TYPE, TrackType::Occupancy))
            {
                Base::currentOverflow += size;
                Base::highestOverflow = std::max(Base::currentOverflow, Base::highestOverflow);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Allocation))
            {
                if (file)
                    Base::allocLogger->Get().trace("[{}:{}] ({}) OverAlloc {} of {} / {} bytes -> {:>}", file, line, allocatorName, count, typeName, size, p);
            }

            if constexpr (Detail::Has(TYPE, TrackType::Size))
            {
                Base::sizeLogger->Get().trace(size);
            }
        }

        void OnOverFlowFree(void* p, size_t size,
                            const char* file, int line, const char* allocatorName, const char* typeName) noexcept
        {
            if constexpr (Detail::Has(TYPE, TrackType::Occupancy))
            {
                Base::currentOverflow -= size;
            }

            if constexpr (Detail::Has(TYPE, TrackType::Allocation))
            {
                if (file)
                    Base::allocLogger->Get().trace("[{}:{}] ({}) OverFree {} / {} bytes -> {:>}", file, line, allocatorName, typeName, size, p);
            }
        }
    };

    template <typename T>
    concept Allocator = requires(T& a)
    {
        { a.Allocate(sizeof(uint32_t), alignof(std::max_align_t)) } noexcept -> std::same_as<void*>;
        
        { a.Free((void*) 0x42, sizeof(uint32_t)) } noexcept -> std::same_as<void>;
    };

    // The stack allocator emulates a global stack, you can get memory from it sequentially.
    // And just like a stack, it requires strict order of free operations.
    // Since there's no bookkeeping and the memory is continuous, this allocator is blazingly fast.
    class StackAllocator
    {
    public:
        // To simplify allocation logic and get rid of any kind of bookkeeping, we require a strong alignment
        static constexpr size_t ALIGNMENT = 2 * alignof(std::max_align_t);
        static_assert((ALIGNMENT & (ALIGNMENT - 1)) == 0u, "Alignment must be power of 2");

    public:
        StackAllocator(const StackAllocator&) = delete;
        StackAllocator& operator=(StackAllocator&&) = delete;
        StackAllocator& operator=(const StackAllocator&) = delete;
    
    public:
        StackAllocator() noexcept = default;

        StackAllocator(void* begin, void* end) noexcept;

        template <AreaType ATYPE>
        explicit StackAllocator(const Area<ATYPE>& area) noexcept
            : StackAllocator(area.Begin(), area.End())
        {}

        void* Allocate(size_t size, size_t alignment = ALIGNMENT) noexcept;

        void Free(void* p, size_t size) noexcept;
    
        void Reset() noexcept;

    private:
        void* m_Begin = nullptr;
        void* m_End = nullptr;
        void* m_Top = nullptr;
    };

    // Allocator with the default heap allocation, a spare when other allocators are full
    class HeapAllocator
    {
    public:
        HeapAllocator(const HeapAllocator&) = delete;
        HeapAllocator& operator=(HeapAllocator&&) = delete;
        HeapAllocator& operator=(const HeapAllocator&) = delete;

    public:
        HeapAllocator() noexcept = default;

        template <AreaType TYPE>
        HeapAllocator(Area<TYPE>& area) noexcept {}

        void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t));

        void Free(void* p, size_t) noexcept;
    };

    template <typename T>
    concept Lock = requires (T& a)
    {
        { a.lock() } noexcept -> std::same_as<void>;
        { a.unlock() } noexcept -> std::same_as<void>;
    };

    class NoLock 
    {
    public:
        void lock() noexcept;
        void unlock() noexcept;
    };

    // The arena is just a collection of allocator (or allocation policy), lock, debug track and memory area
    // This arena owns only one static memory area, which means it can't grow according to needs. So arena has
    // to use a heap allocator as backup.
    template <Allocator ALLOCATOR, Lock LOCK, TrackType TRACK_TYPE, AreaType AREA_TYPE, bool HANDLE_OVERFLOW = true>
    class Arena
    {
        static_assert(!std::is_same_v<ALLOCATOR, HeapAllocator>, "If you want to use default heap allocation, default std::allocator should be enough");

    public:
        Arena() = delete;
        Arena(const Arena&) = delete;
        Arena& operator=(Arena&&) = delete;
        Arena& operator=(const Arena&) = delete;

    public:
        template <typename... Args>
        Arena(std::string_view name, void* begin, void* end, Args&&... args) noexcept
            requires (AREA_TYPE == AreaType::Scope && std::is_constructible_v<ALLOCATOR, Area<AREA_TYPE>&, Args...>)
            : m_Name(name), 
            m_Area(begin, end), 
            m_Alloc(m_Area, std::forward<Args>(args)...),
            m_Track(name, begin, PointerMath::Sub(end, begin))
        {
        }

        template <typename... Args>
        Arena(std::string_view name, size_t size, size_t alignment, Args&&... args) noexcept
            requires (AREA_TYPE == AreaType::Static && std::is_constructible_v<ALLOCATOR, Area<AREA_TYPE>&, Args...>)
            : m_Name(name), 
            m_Area(size, alignment), 
            m_Alloc(m_Area, std::forward<Args>(args)...),
            m_Track(name, m_Area.Begin(), m_Area.Size())
        {
        }

        template <typename... Args>
        Arena(std::string_view name, Area<AREA_TYPE>&& area, Args&&... args) noexcept
            requires (std::is_constructible_v<ALLOCATOR, Area<AREA_TYPE>&, Args...>)
            : m_Name(name), 
            m_Area(std::move(area)), 
            m_Alloc(m_Area, std::forward<Args>(args)...),
            m_Track(name, m_Area.Begin(), m_Area.Size())
        {
        }

        Arena(Arena&& other) noexcept
        {
            std::swap(m_Name, other.m_Name);
            std::swap(m_Alloc, other.m_Alloc);
            std::swap(m_Track, other.m_Track);
            std::swap(m_Area, other.m_Area);
            std::swap(m_Lock, other.m_Lock);
        }

        // Allocate raw pointer.
        // The behavior of this function is determined by the allocator,
        // in some cases, some of these parameters will be simply ignored.
        void* Allocate(size_t size, size_t alignment = alignof(std::max_align_t),
        // parameters to record allocation
                       const char* file = nullptr, int line = 0, const char* allocatorName = nullptr, const char* typeName = nullptr, const size_t count = 0) noexcept
        {
            std::lock_guard guard{ m_Lock };
            void* p = m_Alloc.Allocate(size,  alignment);
            if constexpr (HANDLE_OVERFLOW)
            {
                if (!p) [[unlikely]]
                {
                    p = m_Backup.Allocate(size, alignment);
                    m_Track.OnOverflowAlloc(p, size, alignment, file, line, allocatorName, typeName, count);
                }
                else
                    m_Track.OnAllocation(p, size, alignment, file, line, allocatorName, typeName, count);
            }
            else 
            {
                m_Track.OnAllocation(p, size, alignment, file, line, allocatorName, typeName, count);
            }
            return p;
        }

        void Free(void* p, size_t size,
        // parameters to record free
                  const char* file = nullptr, int line = 0, const char* allocatorName = nullptr, const char* typeName = nullptr) noexcept
        {
            if (m_Area.Inside(p)) [[likely]]
            {
                std::lock_guard guard{ m_Lock };
                m_Track.OnFree(p, size, file, line, allocatorName, typeName);
                m_Alloc.Free(p, size);
                return;
            }
            if constexpr (HANDLE_OVERFLOW)
            {
                std::lock_guard guard{ m_Lock };
                m_Track.OnOverFlowFree(p, size, file, line, allocatorName, typeName);
                m_Backup.Free(p, size);
            }
        }

        std::string_view GetName() const noexcept { return m_Name; }

        ALLOCATOR& GetAllocator() noexcept { return m_Alloc; }

    private:
        std::string_view m_Name;
        Area<AREA_TYPE> m_Area;
        ALLOCATOR m_Alloc;

        LOCK m_Lock;
        Track<TRACK_TYPE> m_Track;
        HeapAllocator m_Backup;
    };

    template <typename T>
    concept ArenaInstance = requires (T& t)
    {
        { t.Allocate((size_t) 0, (size_t) 0, (const char*) nullptr, (int) 0, (const char*) nullptr, (const char*) nullptr, (size_t) 0) } noexcept -> std::same_as<void*>;
        { t.Free((void*) nullptr, (size_t) 0, (const char*) nullptr, (int) 0, (const char*) nullptr, (const char*) nullptr) } noexcept -> std::same_as<void>;
    };

    #define DEF_STLALLOC(TYPE, ARENA, NAME) ::Coust::Memory::STLAllocator<TYPE, std::decay_t<decltype(ARENA)>> NAME{ ARENA };
    #define DEF_STLALLOC_TRACK(TYPE, ARENA, NAME) ::Coust::Memory::STLAllocator<TYPE, std::decay_t<decltype(ARENA)>> NAME{ ARENA, __FILE__, __LINE__, #NAME };

    template <typename T, ArenaInstance A>
    class STLAllocator
    {
    public:
        STLAllocator() = delete;
        STLAllocator(STLAllocator&&) = delete;
        STLAllocator& operator=(STLAllocator&&) = delete;
        STLAllocator& operator=(const STLAllocator&) = delete;

    public:
        using value_type = T;
        using pointer = T*;
        using const_pointer = const T*;
        using reference = T&;
        using const_reference = const T&;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        // We always use stateful allocator (the concept of stateless allocator is weird...)
        using is_always_equal = std::false_type;

    public:
        explicit STLAllocator(A& a, const char* file = nullptr, int line = 0, const char* name = nullptr) noexcept
            : m_A(a), m_CreationFile(file), m_Name(name), m_CreationLine(line)
        {}

        template<typename U>
        STLAllocator(STLAllocator<U, A> const& rhs) noexcept
            : m_A(rhs.m_A), m_CreationFile(rhs.m_CreationFile), m_Name(rhs.m_Name), m_CreationLine(rhs.m_CreationLine)
        {}

        STLAllocator(const STLAllocator&) noexcept = default;

        T* allocate(std::size_t n)  noexcept
        {
            T* p = (T*) m_A.Allocate(n * sizeof(T), alignof(T), 
                m_CreationFile, m_CreationLine, m_Name, typeid(T).name(), sizeof(T));
            return p;
        }

        void deallocate(T* p, std::size_t n) noexcept
        {
            m_A.Free(p, n * sizeof(T), 
                m_CreationFile, m_CreationLine, m_Name, typeid(T).name());
        }

        template <typename U, ArenaInstance AA>
        bool operator==(const STLAllocator<U, AA>& rhs) const noexcept 
        {
            return std::addressof(m_A) == std::addressof(rhs.m_A);
        }

        template <typename U, ArenaInstance AA>
        bool operator!=(const STLAllocator<U, AA>& rhs) const noexcept 
        {
            return !operator==(rhs);
        }

    private:
        template <typename U, ArenaInstance AA>
        friend class STLAllocator;

        A& m_A;

        const char* const m_CreationFile;
        const char* const m_Name;
        const int m_CreationLine;
    };
}