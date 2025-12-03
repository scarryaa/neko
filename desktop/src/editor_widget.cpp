#include "editor_widget.h"
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtDebug>

EditorWidget::EditorWidget(QWidget *parent) : QWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);
}

EditorWidget::~EditorWidget() {}

void EditorWidget::keyPressEvent(QKeyEvent *event) { qDebug() << event->key(); }

void EditorWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  drawText(&painter);
}

void EditorWidget::drawText(QPainter *painter) {
  painter->setBrush(Qt::white);

  painter->drawText(QPointF(20, 20), QString::fromStdString("hello"));
}
