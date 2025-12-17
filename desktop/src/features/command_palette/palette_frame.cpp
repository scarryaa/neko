#include "palette_frame.h"

PaletteFrame::PaletteFrame(neko::ThemeManager &themeManager, QWidget *parent)
    : QFrame(parent), themeManager(themeManager) {
  setAutoFillBackground(false);
  setAttribute(Qt::WA_TranslucentBackground);
}

PaletteFrame::~PaletteFrame() {}

void PaletteFrame::paintEvent(QPaintEvent *) {
  QPainter p(this);
  p.setRenderHint(QPainter::Antialiasing, true);

  QColor fill =
      UiUtils::getThemeColor(themeManager, "command_palette.background");
  QColor stroke =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  QPainterPath path;
  constexpr qreal r = 12.0;
  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), r, r);
  p.fillPath(path, fill);
  QPen pen(stroke, 1.5);
  p.setPen(pen);
  p.drawPath(path);
}
