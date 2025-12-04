#include "editor_widget.h"
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtDebug>
#include <algorithm>
#include <cstddef>
#include <neko_core.h>

EditorWidget::EditorWidget(QWidget *parent)
    : QScrollArea(parent), font(new QFont("IBM Plex Mono", 15.0)),
      fontMetrics(*font) {
  setFocusPolicy(Qt::StrongFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  editor = neko_editor_new();
}

EditorWidget::~EditorWidget() { neko_editor_free(editor); }

double EditorWidget::measureContent() {
  double finalWidth = 0;
  size_t lineCount = 0;
  size_t len;

  neko_editor_get_line_count(editor, &lineCount);

  for (int i = 0; i < lineCount; i++) {
    const char *line = neko_editor_get_line(editor, i, &len);
    QString lineText = QString::fromStdString(line);
    neko_string_free((char *)line);

    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void EditorWidget::handleViewportUpdate() {
  size_t line_count;
  neko_editor_get_line_count(editor, &line_count);

  auto viewportHeight = (line_count * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureContent();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);
}

void EditorWidget::mousePressEvent(QMouseEvent *event) {}

void EditorWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta = event->angleDelta().y() / 8.0;
  double horizontallDelta = event->angleDelta().x() / 8.0;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(newHorizontalScrollOffset);
  verticalScrollBar()->setValue(newVerticalScrollOffset);
  viewport()->repaint();
}

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
    handleViewportUpdate();
    break;
  case Qt::Key_Backspace:
    neko_editor_backspace(editor);
    handleViewportUpdate();
    break;
  case Qt::Key_Delete:
    neko_editor_delete(editor);
    handleViewportUpdate();
    break;
  case Qt::Key_Tab:
    neko_editor_insert_tab(editor);
    handleViewportUpdate();
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
    handleViewportUpdate();
    break;
  case Qt::Key_Minus:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      decreaseFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    handleViewportUpdate();
    break;
  case Qt::Key_0:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      resetFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    handleViewportUpdate();
    break;

  default:
    neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    handleViewportUpdate();
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

  auto verticalOffset = verticalScrollBar()->value();
  auto horizontalOffset = horizontalScrollBar()->value();

  for (int i = 0; i < line_count; i++) {
    size_t len;
    const char *line = neko_editor_get_line(editor, i, &len);
    auto actualY = ((i + 1) * fontMetrics.height()) - verticalOffset;

    painter->drawText(QPointF(-horizontalOffset, actualY),
                      QString::fromStdString(line));

    neko_string_free((char *)line);
  }
}

void EditorWidget::drawCursor(QPainter *painter) {
  painter->setPen(CURSOR_COLOR);

  double lineHeight = fontMetrics.height();
  size_t cursor_row_idx, cursor_col_idx;
  neko_editor_get_cursor_position(editor, &cursor_row_idx, &cursor_col_idx);

  size_t len;
  const char *line = neko_editor_get_line(editor, cursor_row_idx, &len);
  QString lineText = QString::fromStdString(line);
  neko_string_free((char *)line);

  auto verticalOffset = verticalScrollBar()->value();
  auto horizontalOffset = horizontalScrollBar()->value();
  QString textBeforeCursor = lineText.left(cursor_col_idx);
  qreal cursor_x = fontMetrics.horizontalAdvance(textBeforeCursor);

  painter->drawLine(
      QLineF(QPointF(cursor_x - horizontalOffset,
                     (cursor_row_idx * lineHeight) - verticalOffset),
             QPointF(cursor_x - horizontalOffset,
                     ((cursor_row_idx + 1) * lineHeight) - verticalOffset)));
}
