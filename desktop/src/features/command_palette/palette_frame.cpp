#include "palette_frame.h"

PaletteFrame::PaletteFrame(neko::ThemeManager &themeManager, QWidget *parent)
    : QFrame(parent), themeManager(themeManager) {
  setAutoFillBackground(false);
  setAttribute(Qt::WA_TranslucentBackground);
}

void PaletteFrame::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QColor fill =
      UiUtils::getThemeColor(themeManager, "command_palette.background");
  const QColor stroke =
      UiUtils::getThemeColor(themeManager, "command_palette.border");
  QPainterPath path;
  const constexpr qreal radius = 12.0;
  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
  painter.fillPath(path, fill);
  const QPen pen(stroke, 1.5);
  painter.setPen(pen);
  painter.drawPath(path);
}
