#include "editor_widget.h"
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtDebug>
#include <cstddef>
#include <neko_core.h>

EditorWidget::EditorWidget(QWidget *parent)
    : QScrollArea(parent), font(new QFont("IBM Plex Mono", 15.0)),
      fontMetrics(*font) {
  setFocusPolicy(Qt::StrongFocus);

  editor = neko_editor_new();
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

  case Qt::Key_Equal:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      increaseFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    break;
  case Qt::Key_Minus:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      decreaseFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    break;
  case Qt::Key_0:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      resetFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    break;

  default:
    neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    break;
  }

  viewport()->repaint();
}

void EditorWidget::resetFontSize() {
  font->setPointSizeF(DEFAULT_FONT_SIZE);
  fontMetrics = QFontMetricsF(*font);

  viewport()->repaint();
}

void EditorWidget::increaseFontSize() {
  if (font->pointSizeF() < FONT_UPPER_LIMIT) {
    font->setPointSizeF(font->pointSizeF() + FONT_STEP);
    fontMetrics = QFontMetricsF(*font);

    viewport()->repaint();
  }
}

void EditorWidget::decreaseFontSize() {
  if (font->pointSizeF() > FONT_LOWER_LIMIT) {
    font->setPointSizeF(font->pointSizeF() - FONT_STEP);
    fontMetrics = QFontMetricsF(*font);

    viewport()->repaint();
  }
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());
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

  size_t len;
  const char *line = neko_editor_get_line(editor, cursor_row_idx, &len);
  QString lineText = QString::fromStdString(line);
  neko_string_free((char *)line);

  QString textBeforeCursor = lineText.left(cursor_col_idx);
  qreal cursor_x = fontMetrics.horizontalAdvance(textBeforeCursor);

  painter->drawLine(
      QLineF(QPointF(cursor_x, cursor_row_idx * font_size),
             QPointF(cursor_x, (cursor_row_idx + 1) * font_size)));
}
