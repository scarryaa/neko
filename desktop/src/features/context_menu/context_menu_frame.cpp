#include "context_menu_frame.h"

ContextMenuFrame::ContextMenuFrame(const ContextMenuFrameTheme &theme,
                                   QWidget *parent)
    : QFrame(parent) {
  setAutoFillBackground(false);
  setAttribute(Qt::WA_TranslucentBackground);
}

void ContextMenuFrame::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QColor fill = theme.backgroundColor;
  const QColor stroke = theme.borderColor;

  QPainterPath path;
  const constexpr qreal radius = 12.0;
  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), radius, radius);
  painter.fillPath(path, fill);
  const QPen pen(stroke, 1);
  painter.setPen(pen);
  painter.drawPath(path);
}
