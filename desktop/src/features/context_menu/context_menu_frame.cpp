#include "context_menu_frame.h"
#include <QPainter>
#include <QPainterPath>

namespace k {
static const constexpr qreal radius = 12.0;
}

ContextMenuFrame::ContextMenuFrame(const ContextMenuFrameProps &props,
                                   QWidget *parent)
    : QFrame(parent), theme(props.theme) {
  setObjectName("ContextMenuFrame");
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);
  setAttribute(Qt::WA_StyledBackground, false);
  setAttribute(Qt::WA_TranslucentBackground);
}

void ContextMenuFrame::setAndApplyTheme(const ContextMenuFrameTheme &newTheme) {
  theme = newTheme;
  update();
}

void ContextMenuFrame::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing, true);

  const QColor fill = theme.backgroundColor;
  const QColor stroke = theme.borderColor;

  QPainterPath path;
  path.addRoundedRect(rect().adjusted(1, 1, -1, -1), k::radius, k::radius);
  painter.fillPath(path, fill);

  const QPen pen(stroke, 1);
  painter.setPen(pen);
  painter.drawPath(path);
}
