#ifndef EDITOR_TYPES_H
#define EDITOR_TYPES_H

#include <cstdint>

struct Cursor {
  int row;
  int column;
};

struct Selection {
  Cursor start;
  Cursor end;
  Cursor anchor;
  bool active;
};

// TODO(scarlet): Merge with Cursor type
struct RowCol {
  int row;
  int col;
};

namespace ChangeMask {
constexpr uint32_t Buffer = 1 << 0;
constexpr uint32_t Cursor = 1 << 1;
constexpr uint32_t Selection = 1 << 2;
constexpr uint32_t LineCount = 1 << 3;
constexpr uint32_t Widths = 1 << 4;
constexpr uint32_t Viewport = 1 << 5;
} // namespace ChangeMask

#endif
