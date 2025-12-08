#include "editor_widget.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtDebug>
#include <algorithm>
#include <cstddef>
#include <neko_core.h>

EditorWidget::EditorWidget(NekoEditor *editor, QWidget *parent)
    : QScrollArea(parent), editor(editor),
      font(new QFont("IBM Plex Mono", 15.0)), fontMetrics(*font) {
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
      "}"
      "EditorWidget {"
      "  background: black;"
      "}");

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
}

EditorWidget::~EditorWidget() {}

double EditorWidget::measureContent() {
  double finalWidth = 0;
  size_t lineCount = 0;
  size_t len;

  neko_editor_get_line_count(editor, &lineCount);

  // TODO: Make this faster
  for (int i = 0; i < lineCount; i++) {
    const char *line = neko_editor_get_line(editor, i, &len);
    QString lineText = QString::fromStdString(line);
    neko_string_free((char *)line);

    finalWidth = std::max(fontMetrics.horizontalAdvance(lineText), finalWidth);
  }

  return finalWidth;
}

void EditorWidget::scrollToCursor() {
  size_t targetRow, targetCol, len;
  neko_editor_get_cursor_position(editor, &targetRow, &targetCol);

  double lineHeight = fontMetrics.height();
  const char *line = neko_editor_get_line(editor, targetRow, &len);
  QString lineText = QString::fromStdString(line);
  QString textBeforeCursor = lineText.mid(0, targetCol);

  neko_string_free((char *)line);

  double viewportWidth = viewport()->width();
  double viewportHeight = viewport()->height();
  double horizontalScrollOffset = horizontalScrollBar()->value();
  double verticalScrollOffset = verticalScrollBar()->value();

  double targetY = targetRow * lineHeight;
  double targetYBottom = targetY + lineHeight;
  double targetX = fontMetrics.horizontalAdvance(textBeforeCursor);

  if (targetX > viewportWidth - VIEWPORT_PADDING + horizontalScrollOffset) {
    horizontalScrollBar()->setValue(targetX - viewportWidth + VIEWPORT_PADDING);
  } else if (targetX < horizontalScrollOffset + VIEWPORT_PADDING) {
    horizontalScrollBar()->setValue(targetX - VIEWPORT_PADDING);
  }

  if (targetYBottom >
      viewportHeight - VIEWPORT_PADDING + verticalScrollOffset) {
    verticalScrollBar()->setValue(targetYBottom - viewportHeight +
                                  VIEWPORT_PADDING);
  } else if (targetY < verticalScrollOffset + VIEWPORT_PADDING) {
    verticalScrollBar()->setValue(targetY - VIEWPORT_PADDING);
  }
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

void EditorWidget::updateDimensionsAndRepaint() {
  handleViewportUpdate();
  viewport()->repaint();
}

void EditorWidget::mousePressEvent(QMouseEvent *event) {}

void EditorWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  double horizontallDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().x() / 4.0;

  auto newHorizontalScrollOffset = horizontalScrollOffset + horizontallDelta;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(newHorizontalScrollOffset);
  verticalScrollBar()->setValue(newVerticalScrollOffset);
  viewport()->repaint();
}

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  size_t len = event->text().size();
  bool shouldScroll = false;
  bool shouldUpdateViewport = false;
  bool shouldUpdateLineCount = false;

  switch (event->key()) {
  case Qt::Key_Left:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_left(editor);
    } else {
      neko_editor_move_left(editor);
    }
    shouldScroll = true;
    break;
  case Qt::Key_Right:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_right(editor);
    } else {
      neko_editor_move_right(editor);
    }
    shouldScroll = true;
    break;
  case Qt::Key_Up:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_up(editor);
    } else {
      neko_editor_move_up(editor);
    }
    shouldScroll = true;
    break;
  case Qt::Key_Down:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ShiftModifier)) {
      neko_editor_select_down(editor);
    } else {
      neko_editor_move_down(editor);
    }
    shouldScroll = true;
    break;

  case Qt::Key_Enter:
  case Qt::Key_Return:
    neko_editor_insert_newline(editor);
    shouldUpdateViewport = true;
    shouldScroll = true;
    shouldUpdateLineCount = true;
    break;
  case Qt::Key_Backspace:
    neko_editor_backspace(editor);
    shouldUpdateViewport = true;
    shouldScroll = true;
    shouldUpdateLineCount = true;
    break;
  case Qt::Key_Delete:
    neko_editor_delete(editor);
    shouldUpdateViewport = true;
    shouldScroll = true;
    shouldUpdateLineCount = true;
    break;
  case Qt::Key_Tab:
    neko_editor_insert_tab(editor);
    shouldUpdateViewport = true;
    shouldScroll = true;
    break;
  case Qt::Key_Escape:
    neko_editor_clear_selection(editor);
    break;

  case Qt::Key_Equal:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      increaseFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
    }
    shouldUpdateViewport = true;
    break;
  case Qt::Key_Minus:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      decreaseFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
    }
    shouldUpdateViewport = true;
    break;
  case Qt::Key_0:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      resetFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
    }
    shouldUpdateViewport = true;
    break;

  case Qt::Key_A:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      neko_editor_select_all(editor);
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
    }
    shouldUpdateViewport = true;
    break;

  case Qt::Key_C:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      if (neko_editor_get_selection_active(editor)) {
        size_t len;
        char *text = neko_editor_copy(editor, &len);

        if (text) {
          QApplication::clipboard()->setText(QString::fromUtf8(text, len));
          neko_string_free(text);
        }
      }
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    break;
  case Qt::Key_V:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      QString text = QApplication::clipboard()->text();
      QByteArray utf8 = text.toUtf8();
      neko_editor_paste(editor, utf8.constData(), (size_t *)utf8.size());
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    shouldScroll = true;
    shouldUpdateViewport = true;
    shouldUpdateLineCount = true;
    break;
  case Qt::Key_X:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      if (neko_editor_get_selection_active(editor)) {
        size_t len;
        char *text = neko_editor_copy(editor, &len);

        if (text) {
          QApplication::clipboard()->setText(QString::fromUtf8(text, len));
          neko_string_free(text);
        }

        neko_editor_delete(editor);
      }
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    }
    shouldScroll = true;
    shouldUpdateViewport = true;
    shouldUpdateLineCount = true;
    break;

  default:
    if (event->text().isEmpty()) {
      return;
    }

    neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
    shouldUpdateViewport = true;
    shouldScroll = true;
    shouldUpdateLineCount = true;
    break;
  }

  if (shouldUpdateViewport) {
    handleViewportUpdate();
  }

  if (shouldUpdateLineCount) {
    emit lineCountChanged();
  }

  if (shouldScroll) {
    scrollToCursor();
  }

  viewport()->repaint();
}

void EditorWidget::resetFontSize() {
  font->setPointSizeF(DEFAULT_FONT_SIZE);
  fontMetrics = QFontMetricsF(*font);

  viewport()->repaint();
  emit fontSizeChanged(font->pointSizeF());
}

void EditorWidget::increaseFontSize() {
  if (font->pointSizeF() < FONT_UPPER_LIMIT) {
    font->setPointSizeF(font->pointSizeF() + FONT_STEP);
    fontMetrics = QFontMetricsF(*font);

    viewport()->repaint();
    emit fontSizeChanged(font->pointSizeF());
  }
}

void EditorWidget::decreaseFontSize() {
  if (font->pointSizeF() > FONT_LOWER_LIMIT) {
    font->setPointSizeF(font->pointSizeF() - FONT_STEP);
    fontMetrics = QFontMetricsF(*font);

    viewport()->repaint();
    emit fontSizeChanged(font->pointSizeF());
  }
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());
  size_t line_count;

  neko_editor_get_line_count(editor, &line_count);

  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();

  int firstVisibleLine = verticalOffset / lineHeight;
  int visibleLineCount = viewportHeight / lineHeight;
  int lastVisibleLine =
      qMin(firstVisibleLine + visibleLineCount + EXTRA_VERTICAL_LINES,
           (int)line_count);

  ViewportContext ctx = {lineHeight, firstVisibleLine, lastVisibleLine,
                         verticalOffset, horizontalOffset};

  drawText(&painter, ctx);
  drawCursor(&painter, ctx);
  drawSelections(&painter, ctx);
}

void EditorWidget::drawText(QPainter *painter, const ViewportContext &ctx) {
  painter->setPen(TEXT_COLOR);
  painter->setFont(*font);

  for (int line = ctx.firstVisibleLine; line <= ctx.lastVisibleLine; line++) {
    size_t len;
    const char *lineText = neko_editor_get_line(editor, line, &len);

    auto actualY =
        (line * ctx.lineHeight) +
        (ctx.lineHeight + fontMetrics.ascent() - fontMetrics.descent()) / 2.0 -
        ctx.verticalOffset;

    painter->drawText(QPointF(-ctx.horizontalOffset, actualY),
                      QString::fromStdString(lineText));

    neko_string_free((char *)lineText);
  }
}

void EditorWidget::drawSelections(QPainter *painter,
                                  const ViewportContext &ctx) {
  if (!neko_editor_get_selection_active(editor)) {
    return;
  }

  painter->setBrush(SELECTION_COLOR);
  painter->setPen(QColor(0, 0, 0, 0));

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

    double x1 = widthBefore - ctx.horizontalOffset;
    double x2 = widthBefore + width - ctx.horizontalOffset;
    double y1 = (selection_start_row * ctx.lineHeight) - ctx.verticalOffset;
    double y2 =
        ((selection_start_row + 1) * ctx.lineHeight) - ctx.verticalOffset;

    drawSelection(painter, x1, x2, y1, y2);
  } else {
    // Multi line selection
    // First line
    {
      const char *line =
          neko_editor_get_line(editor, selection_start_row, &len);
      auto text = QString(line);

      if (text.isEmpty()) {
        text = QString(" ");
      }

      auto text_length = text.length();

      QString selection_text =
          text.mid(selection_start_col, text_length - selection_start_col);
      QString selection_before_text = text.mid(0, selection_start_col);
      neko_string_free((char *)line);

      double width = fontMetrics.horizontalAdvance(selection_text);
      double widthBefore = fontMetrics.horizontalAdvance(selection_before_text);

      double x1 = widthBefore - ctx.horizontalOffset;
      double x2 = widthBefore + width - ctx.horizontalOffset;
      double y1 = (selection_start_row * ctx.lineHeight) - ctx.verticalOffset;
      double y2 =
          ((selection_start_row + 1) * ctx.lineHeight) - ctx.verticalOffset;

      drawSelection(painter, x1, x2, y1, y2);
    }

    // Middle lines
    {
      for (int i = selection_start_row + 1; i < selection_end_row; i++) {
        const char *line = neko_editor_get_line(editor, i, &len);
        auto text = QString::fromStdString(line);

        neko_string_free((char *)line);

        if (text.isEmpty()) {
          text = QString(" ");
        }

        double x1 = -ctx.horizontalOffset;
        double x2 = fontMetrics.horizontalAdvance(text) - ctx.horizontalOffset;
        double y1 = (i * ctx.lineHeight) - ctx.verticalOffset;
        double y2 = ((i + 1) * ctx.lineHeight) - ctx.verticalOffset;

        drawSelection(painter, x1, x2, y1, y2);
      }
    }

    // Last line
    {
      const char *line = neko_editor_get_line(editor, selection_end_row, &len);
      auto text = QString(line);
      auto text_length = text.length();

      QString selection_text = text.mid(0, selection_end_col);
      neko_string_free((char *)line);

      double width = fontMetrics.horizontalAdvance(selection_text);

      painter->drawRect(QRectF(
          QPointF(0 - ctx.horizontalOffset,
                  selection_end_row * ctx.lineHeight - ctx.verticalOffset),
          QPointF(width - ctx.horizontalOffset,
                  ((selection_end_row + 1) * ctx.lineHeight) -
                      ctx.verticalOffset)));
    }
  }
}

void EditorWidget::drawSelection(QPainter *painter, double x1, double x2,
                                 double y1, double y2) {
  painter->drawRect(QRectF(QPointF(x1, y1), QPointF(x2, y2)));
}

void EditorWidget::drawCursor(QPainter *painter, const ViewportContext &ctx) {
  if (editor == nullptr || !hasFocus()) {
    return;
  }

  painter->setPen(CURSOR_COLOR);

  size_t cursor_row_idx, cursor_col_idx;
  neko_editor_get_cursor_position(editor, &cursor_row_idx, &cursor_col_idx);

  size_t len;
  const char *line = neko_editor_get_line(editor, cursor_row_idx, &len);
  QString lineText = QString::fromStdString(line);
  neko_string_free((char *)line);

  QString textBeforeCursor = lineText.left(cursor_col_idx);
  qreal cursor_x = fontMetrics.horizontalAdvance(textBeforeCursor);

  double topY = (cursor_row_idx * ctx.lineHeight) - ctx.verticalOffset;
  double bottomY = ((cursor_row_idx + 1) * ctx.lineHeight) - ctx.verticalOffset;

  painter->drawLine(QLineF(QPointF(cursor_x - ctx.horizontalOffset, topY),
                           QPointF(cursor_x - ctx.horizontalOffset, bottomY)));
}
