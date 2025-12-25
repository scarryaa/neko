#include "palette_frame.h"
#include <QPainter>
#include <QPainterPath>

PaletteFrame::PaletteFrame(const PaletteFrameProps &props, QWidget *parent)
    : QFrame(parent), theme(props.theme) {
  setFrameShape(QFrame::NoFrame);
  setLineWidth(0);
  setMidLineWidth(0);
  setAutoFillBackground(false);
  setAttribute(Qt::WA_TranslucentBackground);
}

void PaletteFrame::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QColor fill = theme.backgroundColor;
  const QColor stroke = theme.borderColor;

  QPainterPath path;
  const constexpr qreal radius = 12.0;

  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
  painter.fillPath(path, fill);

  const QPen pen(stroke, 1.5);
  painter.setPen(pen);
  painter.drawPath(path);
}

void PaletteFrame::setAndApplyTheme(const PaletteFrameTheme &newTheme) {
  theme = newTheme;

  update();
}
