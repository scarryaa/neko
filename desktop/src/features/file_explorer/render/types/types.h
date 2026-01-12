#ifndef FILE_EXPLORER_RENDERER_TYPES_H
#define FILE_EXPLORER_RENDERER_TYPES_H

#include "theme/types/types.h"
#include <QFont>
#include <QPixmap>
#include <neko-core/src/ffi/bridge.rs.h>

namespace FileExplorerRenderConstants {
static constexpr double selectionAlpha = 60.0;
static constexpr double hoverAlpha = 35.0;
static constexpr double nodeIndent = 20.0;
static constexpr double iconEdgePadding = 12.0;
static constexpr double iconAdjustment = 6.0;
static constexpr double heightDivisor = 2.0;
} // namespace FileExplorerRenderConstants

struct FileExplorerViewportContext {
  double lineHeight;
  int firstVisibleLine;
  int lastVisibleLine;
  double verticalOffset;
  double horizontalOffset;
  double width;
  double height;
};

struct FileExplorerRenderState {
  neko::FileTreeSnapshot snapshot;
  const QFont font;
  const double fontAscent;
  FileExplorerTheme theme;
  const bool hasFocus;
  QString hoveredNodePath;
  const std::function<const double(const QString &str)> measureFileNameWidth;
};

struct IconInfo {
  QPixmap pixmap;
  int size;
};

#endif
