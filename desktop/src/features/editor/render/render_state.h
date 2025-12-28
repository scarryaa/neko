#ifndef RENDER_STATE_H
#define RENDER_STATE_H

#include "features/editor/types/types.h"
#include "render_theme.h"
#include <QFont>
#include <QString>
#include <QStringList>
#include <functional>
#include <neko-core/src/ffi/bridge.rs.h>
#include <vector>

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

#endif // RENDER_STATE_H
