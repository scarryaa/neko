#include "palette_divider.h"
#include <QPainter>

PaletteDivider::PaletteDivider(const QColor &color, QWidget *parent)
    : QWidget(parent), color(color) {
  setFixedHeight(1);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
  setAttribute(Qt::WA_TranslucentBackground);
}

void PaletteDivider::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  QPen pen(color, 1.0);
  pen.setCosmetic(true);
  pen.setCapStyle(Qt::FlatCap);
  pen.setJoinStyle(Qt::MiterJoin);

  painter.setPen(pen);
  painter.drawLine(QPointF(0, height() / HEIGHT_DIVIDER),
                   QPointF(width(), height() / HEIGHT_DIVIDER));
}
