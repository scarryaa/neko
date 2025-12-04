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
  fontMetrics = new QFontMetricsF(*font);
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
  case Qt::Key_Enter:
  case Qt::Key_Return:
    neko_buffer_insert(buffer, idx, "\n", 1);

    size_t row, col;
    neko_cursor_get_position(cursor, &row, &col);
    neko_cursor_set_position(cursor, row + 1, 0);
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
  drawCursor(&painter);
}

void EditorWidget::drawText(QPainter *painter) {
  painter->setBrush(Qt::white);
  painter->setFont(*font);

  size_t line_count;
  neko_buffer_get_line_count(buffer, &line_count);

  for (int i = 0; i < line_count; i++) {
    size_t len;
    const char *line = neko_buffer_get_line(buffer, i, &len);
    painter->drawText(QPointF(0, (i + 1) * font->pointSizeF()),
                      QString::fromStdString(line));

    neko_string_free((char *)line);
  }
}

void EditorWidget::drawCursor(QPainter *painter) {
  painter->setPen(CURSOR_COLOR);

  size_t font_size = font->pointSizeF();
  size_t cursor_row_idx, cursor_col_idx;
  neko_cursor_get_position(cursor, &cursor_row_idx, &cursor_col_idx);

  // TODO: Make this depend on actual measurements (including unicode)
  size_t char_width = fontMetrics->averageCharWidth();

  painter->drawLine(
      QLineF(QPointF(cursor_col_idx * char_width,
                     cursor_row_idx * font_size + CURSOR_WIDTH),
             QPointF(cursor_col_idx * char_width,
                     (cursor_row_idx + 1) * font_size + CURSOR_WIDTH)));
}
