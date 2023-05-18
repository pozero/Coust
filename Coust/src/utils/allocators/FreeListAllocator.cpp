#include "pch.h"

#include "utils/Compiler.h"
#include "utils/Assert.h"
#include "utils/allocators/FreeListAllocator.h"

namespace coust {
namespace memory {

FreeListAllocator::FreeListAllocator(void* begin, void* end) noexcept
    : m_first_free_block((BlockHeader*) begin) {
    size_t const size = ptr_math::sub(end, begin);
    COUST_ASSERT(size < std::numeric_limits<uint32_t>::max(),
        "The size of memory block exceeds the maximum val of `uint32_t`");
    m_first_free_block->size = (uint32_t) size;
    m_first_free_block->next = nullptr;

#if defined(COUST_TEST)
    m_free_block_count = 1;
    m_free_block_size = size;
#endif
}

void* FreeListAllocator::allocate(size_t size, size_t alignment) noexcept {
    COUST_ASSERT(size > 0,
        "You shouldn't allocate memory of size 0 using free list allocator");
    uint32_t constexpr BOOKKEEPING_REQUIREMENT =
        sizeof(BlockHeader) + sizeof(uint32_t);

    BlockHeader* RESTRICT block = m_first_free_block;
    BlockHeader* block_pre = nullptr;
    // best fit means the smallest available memory block
    BlockHeader* RESTRICT best_fit_block = nullptr;
    BlockHeader* RESTRICT best_fit_block_pre = nullptr;
    void* best_fit_available_mem = nullptr;
    uint32_t best_block_available_size = 0;
    while (block) {  // return if reach the end of free list
        BlockHeader* const cur_available_mem = ptr_math::align(
            ptr_math::add(block, BOOKKEEPING_REQUIREMENT), alignment);
        uint32_t const block_size = block->size;
        // prevent underflow
        uint32_t const available_size =
            block_size < (uint32_t) ptr_math::sub(cur_available_mem, block) ?
                0 :
                block_size - (uint32_t) ptr_math::sub(cur_available_mem, block);
        if (available_size >= size &&
            (!best_fit_block || block_size < best_fit_block->size)) {
            best_fit_block = block;
            best_fit_block_pre = block_pre;
            best_fit_available_mem = cur_available_mem;
            best_block_available_size = available_size;
            // if find best possible memory block, early return
            if (available_size == size)
                break;
        }
        block_pre = block;
        block = block->next;
    }
    if (!best_fit_block)
        return nullptr;

    // after get a valid best fit memory block, we try trimming it if there's
    // enough space
    // +--------------------------+
    // |best_block_available_size |
    // +------+-------------------+
    // | size |   residual_size   |
    // +------+-------------------+
    uint32_t const residual_size = best_block_available_size - (uint32_t) size;
    bool const can_be_trimmed = residual_size > BOOKKEEPING_REQUIREMENT + 1;
    // no enough residual space left, give up trimming
    if (can_be_trimmed) {
        BlockHeader* const RESTRICT new_header =
            (BlockHeader*) ptr_math::add(best_fit_available_mem, size);
        new_header->size = residual_size;
        new_header->next = best_fit_block->next;
        best_fit_block->size -= residual_size;
        if (best_fit_block_pre)
            best_fit_block_pre->next = new_header;
        else
            m_first_free_block = new_header;
    } else {
        if (best_fit_block_pre)
            best_fit_block_pre->next = best_fit_block->next;
        else
            m_first_free_block = best_fit_block->next;
    }

#if defined(COUST_TEST)
    if (!can_be_trimmed)
        m_free_block_count--;
    m_free_block_size -= best_fit_block->size;
#endif

    uint32_t* const RESTRICT head =
        (uint32_t*) ptr_math::sub(best_fit_available_mem, sizeof(uint32_t));
    *head = (uint32_t) ptr_math::sub(best_fit_available_mem, best_fit_block);
    return best_fit_available_mem;
}

void FreeListAllocator::deallocate(void* p, [[maybe_unused]] size_t) noexcept {
    uint32_t* const RESTRICT head =
        (uint32_t*) ptr_math::sub(p, sizeof(uint32_t));
    BlockHeader* const RESTRICT free_block_header =
        (BlockHeader*) ptr_math::sub(p, *head);
    free_block_header->next = nullptr;
    void* const free_block_tail =
        ptr_math::add(free_block_header, free_block_header->size);

#if defined(COUST_TEST)
    m_free_block_size += free_block_header->size;
#endif

    // keep the pointing order of free list, which can help defragmenting
    BlockHeader* RESTRICT block = m_first_free_block;
    BlockHeader* RESTRICT block_pre = nullptr;
    // stop when we find two adjacent blocks such that we get the following
    // order: block_pre - free_block - block
    for (; block && block < free_block_tail;
         block_pre = block, block = block->next) {}

    // block and free block aren't adjacent:
    //                    block
    //                      |
    //                      v
    // +------+-------------+-----------+
    //        |             |
    // +------+-------------+-----------+
    //        ^
    //        |
    //  free_block_tail
    // block and free block are next to each other:
    //       block
    //         |
    //         v
    //  +------+-------------+
    //         |
    //  +------+-------------+
    //         ^
    //         |
    //  free_block_tail
    bool const free_block_next_to_block = free_block_tail == block;
    if (block_pre == nullptr) {          // there're only two blocks
        if (free_block_next_to_block) {  // merge free block with block
            free_block_header->size += block->size;
            free_block_header->next = block->next;
        } else  // chain together
            free_block_header->next = block;
        m_first_free_block = free_block_header;

#if defined(COUST_TEST)
        if (!free_block_next_to_block)
            m_free_block_count++;
#endif
    } else {  // there're three blocks
        // previous block and free block aren't adjacent
        //               block_pre + block_pre->size
        //                      |
        //                      v
        // +------+-------------+-----------+
        //        |             |
        // +------+-------------+-----------+
        //        ^
        //        |
        // free_block_header
        // preious block and free block are next to each other
        // block_pre + block_pre->size
        //           |
        //           v
        //    +------+-------------+
        //           |
        //    +------+-------------+
        //           ^
        //           |
        //    free_block_header
        bool const block_pre_next_to_free_block =
            ptr_math::add(block_pre, block_pre->size) == free_block_header;
        if (block_pre_next_to_free_block &&
            free_block_next_to_block) {  // merge three blocks together
            block_pre->size += free_block_header->size + block->size;
            block_pre->next = block->next;
        } else if (block_pre_next_to_free_block) {  // merge pre and free
            block_pre->size += free_block_header->size;
        } else if (free_block_next_to_block) {  // merge free and block
            block_pre->next = free_block_header;
            free_block_header->size += block->size;
            free_block_header->next = block->next;
        } else {  // chain together
            block_pre->next = free_block_header;
            free_block_header->next = block;
        }

#if defined(COUST_TEST)
        if (block_pre_next_to_free_block && free_block_next_to_block) {
            m_free_block_count--;
        } else if (!block_pre_next_to_free_block && !free_block_next_to_block) {
            m_free_block_count++;
        }
#endif
    }
}

void FreeListAllocator::grow(void* p, size_t size) noexcept {
#if defined(COUST_TEST)
    m_free_block_size += size;
    m_free_block_count++;
#endif

    if (!m_first_free_block) {
        m_first_free_block = (BlockHeader*) p;
        m_first_free_block->size = (uint32_t) size;
        m_first_free_block->next = nullptr;
        return;
    }
    BlockHeader* RESTRICT block = m_first_free_block;
    while (block->next) {
        block = block->next;
    }
    block->next = (BlockHeader*) p;
    block->next->size = (uint32_t) size;
    block->next->next = nullptr;
}

#if defined(COUST_TEST)
void FreeListAllocator::is_malfunctioning() const noexcept {
    std::vector<std::pair<BlockHeader*, size_t>> all_free_blocks;
    for (BlockHeader* RESTRICT header = m_first_free_block; header;
         header = header->next) {
        all_free_blocks.push_back({header, header->size});
    }
    COUST_PANIC_IF_NOT(m_free_block_count == all_free_blocks.size(), "");
    for (size_t i = 1; i < all_free_blocks.size(); ++i) {
        COUST_PANIC_IF_NOT(
            all_free_blocks[i - 1].first < all_free_blocks[i].first, "");
    }
    auto const free_block_size =
        std::accumulate(all_free_blocks.begin(), all_free_blocks.end(),
            size_t{0}, [](size_t val, auto size) { return val + size.second; });
    COUST_PANIC_IF_NOT(free_block_size == m_free_block_size, "");
}
#endif

}  // namespace memory
}  // namespace coust
