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
  setCornerWidget(nullptr);

  setStyleSheet(
      "QAbstractScrollArea::corner {"
      "  background: transparent;"
      "}"
      "QScrollBar:vertical {"
      "  background: transparent;"
      "  width: 12px;"
      "  margin: 0px;"
      "}"
      "QScrollBar::handle:vertical {"
      "  background: #555555;"
      "  min-height: 20px;"
      "}"
      "QScrollBar::handle:vertical:hover {"
      "  background: #666666;"
      "}"
      "QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {"
      "  height: 0px;"
      "}"
      "QScrollBar::add-page:vertical, QScrollBar::sub-page:vertical {"
      "  background: none;"
      "}"
      "QScrollBar:horizontal {"
      "  background: transparent;"
      "  height: 12px;"
      "  margin: 0px;"
      "}"
      "QScrollBar::handle:horizontal {"
      "  background: #555555;"
      "  min-width: 20px;"
      "}"
      "QScrollBar::handle:horizontal:hover {"
      "  background: #666666;"
      "}"
      "QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {"
      "  width: 0px;"
      "}"
      "QScrollBar::add-page:horizontal, QScrollBar::sub-page:horizontal {"
      "  background: none;"
      "}");

  editor = neko_editor_new();

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
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
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_left(editor);
    } else {
      neko_editor_move_left(editor);
    }
    break;
  case Qt::Key_Right:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_right(editor);
    } else {
      neko_editor_move_right(editor);
    }
    break;
  case Qt::Key_Up:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_up(editor);
    } else {
      neko_editor_move_up(editor);
    }
    break;
  case Qt::Key_Down:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_down(editor);
    } else {
      neko_editor_move_down(editor);
    }
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
    neko_editor_clear_selection(editor);
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

  case Qt::Key_A:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      neko_editor_select_all(editor);
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    handleViewportUpdate();
    break;

  default:
    if (event->text().isEmpty()) {
      return;
    }

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
  drawSelection(&painter);
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

    auto actualY =
        (i * fontMetrics.height()) +
        (fontMetrics.height() + fontMetrics.ascent() - fontMetrics.descent()) /
            2.0 -
        verticalOffset;

    painter->drawText(QPointF(-horizontalOffset, actualY),
                      QString::fromStdString(line));

    neko_string_free((char *)line);
  }
}

void EditorWidget::drawSelection(QPainter *painter) {
  if (!neko_editor_get_selection_active(editor)) {
    return;
  }

  painter->setBrush(SELECTION_COLOR);
  painter->setPen(QColor(0, 0, 0, 0));

  double lineHeight = fontMetrics.height();
  size_t lineCount;
  size_t len;
  neko_editor_get_line_count(editor, &lineCount);

  size_t selection_start_row, selection_start_col, selection_end_row,
      selection_end_col;
  neko_editor_get_selection_start(editor, &selection_start_row,
                                  &selection_start_col);
  neko_editor_get_selection_end(editor, &selection_end_row, &selection_end_col);

  if (selection_start_row == selection_end_row) {
    // Single line selection
    const char *line = neko_editor_get_line(editor, selection_start_row, &len);
    auto text = QString(line);
    QString selection_text =
        text.mid(selection_start_col, selection_end_col - selection_start_col);
    QString selection_before_text = text.mid(0, selection_start_col);
    neko_string_free((char *)line);

    double width = fontMetrics.horizontalAdvance(selection_text);
    double widthBefore = fontMetrics.horizontalAdvance(selection_before_text);

    painter->drawRect(
        QRectF(QPointF(widthBefore, selection_start_row * lineHeight),
               QPointF(widthBefore + width,
                       ((selection_start_row + 1) * lineHeight))));
  } else {
    // Multi line selection
    // First line
    const char *line = neko_editor_get_line(editor, selection_start_row, &len);
    auto text = QString(line);
    auto text_length = text.length();

    QString selection_text =
        text.mid(selection_start_col, text_length - selection_start_col);
    QString selection_before_text = text.mid(0, selection_start_col);
    neko_string_free((char *)line);

    double width = fontMetrics.horizontalAdvance(selection_text);
    double widthBefore = fontMetrics.horizontalAdvance(selection_before_text);

    painter->drawRect(
        QRectF(QPointF(widthBefore, selection_start_row * lineHeight),
               QPointF(widthBefore + width,
                       ((selection_start_row + 1) * lineHeight))));

    // Middle lines
    for (int i = selection_start_row + 1; i < selection_end_row; i++) {
      const char *line = neko_editor_get_line(editor, i, &len);

      auto text = QString::fromStdString(line);
      neko_string_free((char *)line);
      double x1 = fontMetrics.horizontalAdvance(text);

      painter->drawRect(QRectF(QPointF(0, i * lineHeight),
                               QPointF(x1, ((i + 1) * lineHeight))));
    }

    // Last line
    {
      const char *line = neko_editor_get_line(editor, selection_end_row, &len);
      auto text = QString(line);
      auto text_length = text.length();

      QString selection_text = text.mid(0, selection_end_col);
      neko_string_free((char *)line);

      double width = fontMetrics.horizontalAdvance(selection_text);

      painter->drawRect(
          QRectF(QPointF(0, selection_end_row * lineHeight),
                 QPointF(width, ((selection_end_row + 1) * lineHeight))));
    }
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

  double topY = (cursor_row_idx * lineHeight) - verticalOffset;
  double bottomY = ((cursor_row_idx + 1) * lineHeight) - verticalOffset;

  painter->drawLine(QLineF(QPointF(cursor_x - horizontalOffset, topY),
                           QPointF(cursor_x - horizontalOffset, bottomY)));
}
