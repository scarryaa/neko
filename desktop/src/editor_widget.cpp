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
  cursor = neko_cursor_new();
  font = new QFont("IBM Plex Mono", 15.0);
}

EditorWidget::~EditorWidget() {
  neko_buffer_free(buffer);
  neko_cursor_free(cursor);
}

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  size_t idx;
  neko_cursor_get_idx(cursor, buffer, &idx);
  size_t len = event->text().size();

  switch (event->key()) {
  case Qt::Key_Left:
    neko_cursor_move_left(cursor, buffer);
    break;
  case Qt::Key_Right:
    neko_cursor_move_right(cursor, buffer);
    break;
  case Qt::Key_Up:
    neko_cursor_move_up(cursor, buffer);
    break;
  case Qt::Key_Down:
    neko_cursor_move_down(cursor, buffer);
    break;
  default:
    neko_buffer_insert(buffer, idx, event->text().toStdString().c_str(), len);
    neko_cursor_move_right(cursor, buffer);
    break;
  }

  repaint();
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  drawText(&painter);
}

void EditorWidget::drawText(QPainter *painter) {
  painter->setBrush(Qt::white);
  painter->setFont(*font);

  size_t len;
  const char *text = neko_buffer_get_text(buffer, &len);
  painter->drawText(QPointF(0, font->pointSizeF()),
                    QString::fromStdString(text));
  neko_string_free((char *)text);
}
