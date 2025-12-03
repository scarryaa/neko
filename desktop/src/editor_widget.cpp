#include "editor_widget.h"
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtDebug>
#include <cstddef>
#include <neko_core.h>

EditorWidget::EditorWidget(QWidget *parent) : QWidget(parent) {
  setFocusPolicy(Qt::StrongFocus);

  buffer = neko_buffer_new();
}

EditorWidget::~EditorWidget() {}

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  neko_buffer_insert(buffer, 0, event->text().toStdString().c_str(), 1);
  repaint();
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  drawText(&painter);
}

void EditorWidget::drawText(QPainter *painter) {
  painter->setBrush(Qt::white);

  size_t len;
  painter->drawText(QPointF(20, 20),
                    QString::fromStdString(neko_buffer_get_text(buffer, &len)));
}
