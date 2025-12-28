#ifndef EDITOR_RENDERER_TYPES_H
#define EDITOR_RENDERER_TYPES_H

#include "features/editor/types/types.h"
#include <QFont>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <functional>
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

struct ViewportContext {
  double lineHeight;
  int firstVisibleLine;
  int lastVisibleLine;
  double verticalOffset;
  double horizontalOffset;
  double width;
  double height;
};

struct RenderTheme {
  QString textColor;
  QString activeLineTextColor;
  QString accentColor;
  QString highlightColor;
};

struct RenderState {
  const QStringList lines;
  const std::vector<Cursor> cursors;
  const Selection selections;
  const RenderTheme theme;
  const int lineCount;
  const double verticalOffset;
  const double horizontalOffset;
  const double lineHeight;
  const double fontAscent;
  const double fontDescent;
  const QFont font;
  const bool hasFocus;
  const bool isEmpty;

  const std::function<const double(const QString &str)> measureWidth;
};

#endif
