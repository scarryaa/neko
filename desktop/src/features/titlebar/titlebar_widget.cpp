#include "titlebar_widget.h"
#include <QMouseEvent>
#include <QPainter>

TitleBarWidget::TitleBarWidget(QWidget *parent) : QWidget(parent) {
  setFixedHeight(TITLEBAR_HEIGHT);
  setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
}

TitleBarWidget::~TitleBarWidget() {}

void TitleBarWidget::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    m_clickPos = event->position().toPoint();
  }

  QWidget::mousePressEvent(event);
}

void TitleBarWidget::mouseMoveEvent(QMouseEvent *event) {
  if (event->buttons() & Qt::LeftButton) {
    window()->move((event->globalPosition() - m_clickPos).toPoint());

    event->accept();
  }
}

void TitleBarWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setBrush(COLOR_BLACK);
  painter.setPen(Qt::NoPen);

  painter.drawRect(QRectF(QPointF(0, 0), QPointF(width(), TITLEBAR_HEIGHT)));

  painter.setPen(QPen(BORDER_COLOR));
  painter.drawLine(QPointF(0, height() - 1), QPointF(width(), height() - 1));
}
