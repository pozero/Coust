#pragma once

#include "utils/Compiler.h"

WARNING_PUSH
DISABLE_ALL_WARNING
#include "volk.h"
WARNING_POP

namespace coust {
namespace render {

uint32_t constexpr MAX_ATTACHMENT_COUNT = 8;

using AttachmentFlags = uint32_t;
enum AttachmentFlagBits : AttachmentFlags {
    // support at most 8 color attachment (from filament)
    // then we can use several single uint8_t masks to specify their proerties
    NONE = 0X0,
    COLOR0 = (1 << 0),
    COLOR1 = (1 << 1),
    COLOR2 = (1 << 2),
    COLOR3 = (1 << 3),
    COLOR4 = (1 << 4),
    COLOR5 = (1 << 5),
    COLOR6 = (1 << 6),
    COLOR7 = (1 << 7),
    DEPTH = (1 << 8),
    COLOR_ALL =
        COLOR0 | COLOR1 | COLOR2 | COLOR3 | COLOR4 | COLOR5 | COLOR6 | COLOR7,
    ALL = COLOR_ALL | DEPTH,
};

}  // namespace render
}  // namespace coust
