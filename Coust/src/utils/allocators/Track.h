#pragma once

#include "utils/allocators/Allocator.h"

#include "utils/AlignedStorage.h"
#include "core/Logger.h"

namespace coust {
namespace memory {

enum class TrackType {
    none = 0,
    // Occupancy track can report the highest occupancy rate and overflow size
    // and we can also check current occupance at the destructor to indicate
    // memory leak
    occupancy = 1,
    // Access track will set the memory to specific value when other codes
    // manipulating them, so we can tell what happened before some memory
    // bugs show up.
    access = 2,
    // Record all allocation size
    // Useful for set appropriate memory block size
    size = 4,

    all = occupancy | access | size,
};

namespace detail {

struct TrackParamBase {
    const std::string_view name{};
    void* const base = nullptr;
    size_t const size = 0;
};

struct TrackParamOccupance {
    // occupance
    size_t cur_op = 0;
    size_t high_op = 0;
    // overflow
    size_t cur_of = 0;
    size_t high_of = 0;
};

struct TrackParamSize {
    AlignedStorage<coust::Logger> size_logger;
};

}  // namespace detail
}  // namespace memory
}  // namespace coust
