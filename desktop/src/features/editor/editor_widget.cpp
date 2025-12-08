#include "editor_widget.h"
#include <QApplication>
#include <QClipboard>
#include <QKeyEvent>
#include <QLabel>
#include <QPainter>
#include <QVBoxLayout>
#include <QtDebug>
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
  size_t lineCount = 0;
  neko_editor_get_line_count(editor, &lineCount);

  for (size_t i = 0; i < lineCount; i++) {
    if (neko_editor_needs_width_measurement(editor, i)) {
      size_t len;
      const char *line(neko_editor_get_line(editor, i, &len));
      double width = fontMetrics.horizontalAdvance(QString(line));
      neko_editor_set_line_width(editor, i, width);

      neko_string_free((char *)line);
    }
  }

  return neko_editor_get_max_width(editor);
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

bool EditorWidget::focusNextPrevChild(bool next) { return false; }

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
      shouldUpdateLineCount = true;
    }
    shouldUpdateViewport = true;
    break;
  case Qt::Key_Minus:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      decreaseFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
      shouldUpdateLineCount = true;
    }
    shouldUpdateViewport = true;
    break;
  case Qt::Key_0:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      resetFontSize();
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
      shouldUpdateLineCount = true;
    }
    shouldUpdateViewport = true;
    break;

  case Qt::Key_A:
    if (event->modifiers().testFlag(Qt::KeyboardModifier::ControlModifier)) {
      neko_editor_select_all(editor);
    } else {
      neko_editor_insert_text(editor, event->text().toStdString().c_str(), len);
      shouldScroll = true;
      shouldUpdateLineCount = true;
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
      shouldUpdateViewport = true;
      shouldScroll = true;
      shouldUpdateLineCount = true;
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
      shouldUpdateViewport = true;
      shouldScroll = true;
      shouldUpdateLineCount = true;
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

double EditorWidget::getLineTopY(size_t lineIndex,
                                 const ViewportContext &ctx) const {
  return (lineIndex * ctx.lineHeight) - ctx.verticalOffset;
}

double EditorWidget::getLineBottomY(size_t lineIndex,
                                    const ViewportContext &ctx) const {
  return ((lineIndex + 1) * ctx.lineHeight) - ctx.verticalOffset;
}

double EditorWidget::getTextWidth(const QString &text,
                                  double horizontalOffset) const {
  return fontMetrics.horizontalAdvance(text) - horizontalOffset;
}

QRectF EditorWidget::getLineRect(size_t lineIndex, double x1, double x2,
                                 const ViewportContext &ctx) const {
  return QRectF(QPointF(x1, getLineTopY(lineIndex, ctx)),
                QPointF(x2, getLineBottomY(lineIndex, ctx)));
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
  painter->setPen(Qt::transparent);

  size_t startRow, startCol, endRow, endCol;
  neko_editor_get_selection_start(editor, &startRow, &startCol);
  neko_editor_get_selection_end(editor, &endRow, &endCol);

  if (startRow == endRow) {
    drawSingleLineSelection(painter, ctx, startRow, startCol, endCol);
  } else {
    drawFirstLineSelection(painter, ctx, startRow, startCol);
    drawMiddleLinesSelection(painter, ctx, startRow, endRow);
    drawLastLineSelection(painter, ctx, endRow, endCol);
  }
}

void EditorWidget::drawSingleLineSelection(QPainter *painter,
                                           const ViewportContext &ctx,
                                           size_t startRow, size_t startCol,
                                           size_t endCol) {
  size_t len;
  const char *line = neko_editor_get_line(editor, startRow, &len);
  QString text = QString(line);

  QString selection_text = text.mid(startCol, endCol - startCol);
  QString selection_before_text = text.mid(0, startCol);

  neko_string_free((char *)line);

  double width = fontMetrics.horizontalAdvance(selection_text);
  double widthBefore = fontMetrics.horizontalAdvance(selection_before_text);

  double x1 = widthBefore - ctx.horizontalOffset;
  double x2 = widthBefore + width - ctx.horizontalOffset;

  painter->drawRect(getLineRect(startRow, x1, x2, ctx));
}

void EditorWidget::drawFirstLineSelection(QPainter *painter,
                                          const ViewportContext &ctx,
                                          size_t startRow, size_t startCol) {
  if (startRow > ctx.lastVisibleLine || startRow < ctx.firstVisibleLine) {
    return;
  }

  size_t len;
  const char *line = neko_editor_get_line(editor, startRow, &len);
  QString text = QString(line);

  if (text.isEmpty())
    text = " ";

  neko_string_free((char *)line);

  QString selectionText = text.mid(startCol);
  QString beforeText = text.mid(0, startCol);

  double widthBefore = fontMetrics.horizontalAdvance(beforeText);
  double width = fontMetrics.horizontalAdvance(selectionText);

  double x1 = widthBefore - ctx.horizontalOffset;
  double x2 = widthBefore + width - ctx.horizontalOffset;

  painter->drawRect(getLineRect(startRow, x1, x2, ctx));
}

void EditorWidget::drawMiddleLinesSelection(QPainter *painter,
                                            const ViewportContext &ctx,
                                            size_t startRow, size_t endRow) {
  for (size_t i = startRow + 1; i < endRow; i++) {
    if (i > ctx.lastVisibleLine || i < ctx.firstVisibleLine) {
      continue;
    }

    size_t len;
    const char *line = neko_editor_get_line(editor, i, &len);
    QString text = QString(line);

    if (text.isEmpty())
      text = " ";

    neko_string_free((char *)line);

    double x1 = -ctx.horizontalOffset;
    double x2 = fontMetrics.horizontalAdvance(text) - ctx.horizontalOffset;

    painter->drawRect(getLineRect(i, x1, x2, ctx));
  }
}

void EditorWidget::drawLastLineSelection(QPainter *painter,
                                         const ViewportContext &ctx,
                                         size_t endRow, size_t endCol) {
  if (endRow > ctx.lastVisibleLine || endRow < ctx.firstVisibleLine) {
    return;
  }

  size_t len;
  const char *line = neko_editor_get_line(editor, endRow, &len);
  QString text = QString(line);
  QString selectionText = text.mid(0, endCol);

  neko_string_free((char *)line);

  double width = fontMetrics.horizontalAdvance(selectionText);

  painter->drawRect(getLineRect(endRow, -ctx.horizontalOffset,
                                width - ctx.horizontalOffset, ctx));
}

void EditorWidget::drawCursor(QPainter *painter, const ViewportContext &ctx) {
  if (!editor || !hasFocus()) {
    return;
  }

  size_t cursorRow, cursorCol;
  neko_editor_get_cursor_position(editor, &cursorRow, &cursorCol);

  if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
    return;
  }

  size_t len;
  const char *line = neko_editor_get_line(editor, cursorRow, &len);
  QString textBeforeCursor = QString(line).left(cursorCol);

  qreal cursorX = fontMetrics.horizontalAdvance(textBeforeCursor);

  neko_string_free((char *)line);

  if (cursorX < 0 || cursorX > viewport()->width() + ctx.horizontalOffset) {
    return;
  }

  painter->setPen(CURSOR_COLOR);

  double x = cursorX - ctx.horizontalOffset;
  painter->drawLine(QLineF(QPointF(x, getLineTopY(cursorRow, ctx)),
                           QPointF(x, getLineBottomY(cursorRow, ctx))));
}
