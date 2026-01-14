#ifndef FILE_EXPLORER_TYPES
#define FILE_EXPLORER_TYPES

#include <cstdint>
#include <neko-core/src/ffi/bridge.rs.h>
#include <string>

enum class FontSizeAdjustment : uint8_t {
  NoChange,
  Increase,
  Decrease,
  Reset,
};

struct ChangeSet {
  bool scroll;
  bool redraw;
  FontSizeAdjustment fontSizeAdjustment;
};

struct FileNodeInfo {
  int index = -1;
  neko::FileNodeSnapshot nodeSnapshot;

  [[nodiscard]] bool foundNode() const { return index != -1; }
};

struct TargetNodePath {
  std::string value;
  explicit TargetNodePath(std::string val) : value(std::move(val)) {}
};

struct DestinationNodePath {
  std::string value;
  explicit DestinationNodePath(std::string val) : value(std::move(val)) {}
};

#endif
