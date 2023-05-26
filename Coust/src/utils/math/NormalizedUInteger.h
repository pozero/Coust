#pragma once

namespace coust {

inline float ubyte_to_float(uint8_t val) noexcept {
    return float(val) * (1 / float(std::numeric_limits<uint8_t>::max()));
}

inline float ushort_to_float(uint16_t val) noexcept {
    return float(val) * (1 / float(std::numeric_limits<uint16_t>::max()));
}

}  // namespace coust
