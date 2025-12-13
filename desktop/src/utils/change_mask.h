#ifndef CHANGE_MASK_H
#define CHANGE_MASK_H

#include <cstdint>

namespace ChangeMask {
constexpr uint32_t Buffer = 1 << 0;
constexpr uint32_t Cursor = 1 << 1;
constexpr uint32_t Selection = 1 << 2;
constexpr uint32_t LineCount = 1 << 3;
constexpr uint32_t Widths = 1 << 4;
constexpr uint32_t Viewport = 1 << 5;
} // namespace ChangeMask

#endif // CHANGE_MASK_H
