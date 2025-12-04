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

  editor = neko_editor_new();
  font = new QFont("IBM Plex Mono", 15.0);
  fontMetrics = new QFontMetricsF(*font);
}

EditorWidget::~EditorWidget() { neko_editor_free(editor); }

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  size_t len = event->text().size();

  switch (event->key()) {
  case Qt::Key_Left:
    neko_editor_move_left(editor);
    break;
  case Qt::Key_Right:
    neko_editor_move_right(editor);
    break;
  case Qt::Key_Up:
    neko_editor_move_up(editor);
    break;
  case Qt::Key_Down:
    neko_editor_move_down(editor);
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    neko_editor_insert_newline(editor);
    break;
  case Qt::Key_Backspace:
    neko_editor_backspace(editor);
    break;
  case Qt::Key_Delete:
    neko_editor_delete(editor);
    break;
  case Qt::Key_Tab:
    neko_editor_insert_tab(editor);
    break;
  case Qt::Key_Escape:
    // TODO: Clear selection
    break;
  default:
    neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
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
  neko_editor_get_line_count(editor, &line_count);

  for (int i = 0; i < line_count; i++) {
    size_t len;
    const char *line = neko_editor_get_line(editor, i, &len);
    painter->drawText(QPointF(0, (i + 1) * font->pointSizeF()),
                      QString::fromStdString(line));

    neko_string_free((char *)line);
  }
}

void EditorWidget::drawCursor(QPainter *painter) {
  painter->setPen(CURSOR_COLOR);

  size_t font_size = font->pointSizeF();
  size_t cursor_row_idx, cursor_col_idx;
  neko_editor_get_cursor_position(editor, &cursor_row_idx, &cursor_col_idx);

  // TODO: Make this depend on actual measurements (including unicode)
  size_t char_width = fontMetrics->averageCharWidth();

  painter->drawLine(
      QLineF(QPointF(cursor_col_idx * char_width,
                     cursor_row_idx * font_size + CURSOR_WIDTH),
             QPointF(cursor_col_idx * char_width,
                     (cursor_row_idx + 1) * font_size + CURSOR_WIDTH)));
}
