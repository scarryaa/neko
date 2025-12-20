#ifndef RENDER_STATE_H
#define RENDER_STATE_H

#include "render_theme.h"
#include <QFont>
#include <neko-core/src/ffi/mod.rs.h>

struct RenderState {
  rust::Vec<rust::String> rawLines;
  rust::Vec<neko::CursorPosition> cursors;
  neko::Selection selections;
  RenderTheme theme;
  int lineCount;
  double verticalOffset;
  double horizontalOffset;
  double lineHeight;
  double fontAscent;
  double fontDescent;
  QFont font;
  bool hasFocus;
  bool isEmpty;

  std::function<double(QString str)> measureWidth;
};

#endif // RENDER_STATE_H
