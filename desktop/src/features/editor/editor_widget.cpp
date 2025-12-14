#include "editor_widget.h"
#include <neko-core/src/ffi/mod.rs.h>

EditorWidget::EditorWidget(neko::Editor *editor,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), editor(editor), configManager(configManager),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, neko::FontType::Editor)),
      fontMetrics(font) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  QString bgHex =
      UiUtils::getThemeColor(themeManager, "editor.background", "#000000");

  setStyleSheet(UiUtils::getScrollBarStylesheet("EditorWidget", bgHex));

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->update(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->update(); });
}

EditorWidget::~EditorWidget() {}

void EditorWidget::applyChangeSet(const neko::ChangeSetFfi &cs) {
  const uint32_t m = cs.mask;

  if (m & (ChangeMask::Cursor | ChangeMask::Selection)) {
    emit cursorPositionChanged();
  }

  if (m & (ChangeMask::Viewport | ChangeMask::LineCount | ChangeMask::Widths)) {
    handleViewportUpdate();
  }

  if (m & ChangeMask::LineCount) {
    emit lineCountChanged();
  }

  if (m & ChangeMask::Cursor) {
    scrollToCursor();
  }

  if (m & ChangeMask::Buffer) {
    emit bufferChanged();
  }

  viewport()->update();
}

void EditorWidget::setEditor(neko::Editor *newEditor) { editor = newEditor; }

double EditorWidget::measureContent() {
  if (editor == nullptr) {
    return 0;
  }

  size_t lineCount = editor->get_line_count();

  for (size_t i = 0; i < lineCount; i++) {
    if (editor->needs_width_measurement(i)) {
      auto rawLine = editor->get_line(i);
      QString line = QString::fromUtf8(rawLine);
      double width = fontMetrics.horizontalAdvance(line);
      editor->set_line_width(i, width);
    }
  }

  return editor->get_max_width();
}

void EditorWidget::scrollToCursor() {
  if (editor == nullptr) {
    return;
  }

  auto cursors = editor->get_cursor_positions();
  auto cursor = cursors.at(0);

  int targetRow = cursor.row;
  double lineHeight = fontMetrics.height();

  auto rawLine = editor->get_line(cursor.row);
  QString line = QString::fromUtf8(rawLine);
  QString textBeforeCursor = line.mid(0, cursor.col);

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
  if (editor == nullptr) {
    return;
  }

  int lineCount = editor->get_line_count();

  auto viewportHeight = (lineCount * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureContent();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  bool horizontalScrollBarVisible = horizontalScrollBar()->isVisible();
  double horizontalScrollBarHeight = horizontalScrollBar()->height();
  double adjustedVerticalRange =
      viewportHeight -
      (horizontalScrollBarVisible ? horizontalScrollBarHeight : 0.0);

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, adjustedVerticalRange);
}

void EditorWidget::updateDimensionsAndRepaint() {
  handleViewportUpdate();
  viewport()->update();
}

void EditorWidget::mousePressEvent(QMouseEvent *event) {
  if (editor == nullptr) {
    return;
  }

  RowCol rc = convertMousePositionToRowCol(event->pos().x(), event->pos().y());

  if (event->modifiers().testFlag(Qt::AltModifier)) {
    editor->add_cursor(rc.row, rc.col);
  } else {
    editor->move_to(rc.row, rc.col, true);
  }

  emit cursorPositionChanged();
  viewport()->update();
}

void EditorWidget::mouseMoveEvent(QMouseEvent *event) {
  if (editor == nullptr) {
    return;
  }

  if (event->buttons() == Qt::LeftButton) {
    RowCol rc =
        convertMousePositionToRowCol(event->pos().x(), event->pos().y());

    editor->select_to(rc.row, rc.col);
    emit cursorPositionChanged();
    viewport()->update();
  }
}

static int xToCursorIndex(const QString &line, const QFont &font, qreal x) {
  QTextLayout layout(line, font);
  layout.beginLayout();

  QTextLine tl = layout.createLine();

  if (!tl.isValid())
    return 0;

  tl.setLineWidth(1e9);

  layout.endLayout();

  return tl.xToCursor(x);
}

RowCol EditorWidget::convertMousePositionToRowCol(double x, double y) {
  const double lineHeight = fontMetrics.height();
  const int scrollX = horizontalScrollBar()->value();
  const int scrollY = verticalScrollBar()->value();

  const int lineCount = editor->get_line_count();
  if (lineCount <= 0)
    return {0, 0};

  const size_t targetRow = size_t((y + scrollY) / lineHeight);
  const size_t lastRow = size_t(lineCount - 1);
  const size_t row = std::min(targetRow, lastRow);

  const QString line = QString::fromUtf8(editor->get_line(row));
  const qreal targetX = qreal(x + scrollX);

  int col = xToCursorIndex(line, font, targetX);
  col = std::clamp(col, 0, (int)line.length());

  return {(int)row, col};
}

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
  viewport()->update();
}

bool EditorWidget::focusNextPrevChild(bool next) { return false; }

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  if (editor == nullptr) {
    return;
  }

  const auto mods = event->modifiers();
  const bool ctrl = mods.testFlag(Qt::ControlModifier);
  const bool shift = mods.testFlag(Qt::ShiftModifier);

  auto nav = [&](auto moveFn, auto selectFn) -> neko::ChangeSetFfi {
    if (shift)
      return (editor->*selectFn)();
    else
      return (editor->*moveFn)();
  };

  if (ctrl) {
    switch (event->key()) {
    case Qt::Key_A:
      applyChangeSet(editor->select_all());
      return;
    case Qt::Key_C:
      handleCopy();
      return;
    case Qt::Key_V: {
      handlePaste();
      return;
    }
    case Qt::Key_X:
      handleCut();
      return;

    case Qt::Key_Z:
      if (shift) {
        handleRedo();
      } else {
        handleUndo();
      }
      return;
    case Qt::Key_Equal:
      increaseFontSize();
      handleViewportUpdate();
      return;
    case Qt::Key_Minus:
      decreaseFontSize();
      handleViewportUpdate();
      return;
    case Qt::Key_0:
      resetFontSize();
      handleViewportUpdate();
      return;
    }
  }

  switch (event->key()) {
  case Qt::Key_Left:
    applyChangeSet(nav(&neko::Editor::move_left, &neko::Editor::select_left));
    break;
  case Qt::Key_Right:
    applyChangeSet(nav(&neko::Editor::move_right, &neko::Editor::select_right));
    break;
  case Qt::Key_Up:
    applyChangeSet(nav(&neko::Editor::move_up, &neko::Editor::select_up));
    break;
  case Qt::Key_Down:
    applyChangeSet(nav(&neko::Editor::move_down, &neko::Editor::select_down));
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    applyChangeSet(editor->insert_newline());
    break;
  case Qt::Key_Backspace:
    applyChangeSet(editor->backspace());
    break;
  case Qt::Key_Delete:
    applyChangeSet(editor->delete_forwards());
    break;
  case Qt::Key_Tab:
    applyChangeSet(editor->insert_tab());
    break;
  case Qt::Key_Escape:
    applyChangeSet(editor->clear_selection());
    break;

  default:
    if (event->text().isEmpty()) {
      return;
    }

    applyChangeSet(editor->insert_text(event->text().toStdString()));
    break;
  }
}

void EditorWidget::handleCopy() {
  if (editor->get_selection().active) {
    auto rawText = editor->copy();
    QString text = QString::fromUtf8(rawText);

    if (!text.isEmpty()) {
      QApplication::clipboard()->setText(text);
    }
  }
}

void EditorWidget::handlePaste() {
  QString text = QApplication::clipboard()->text();
  applyChangeSet(editor->paste(text.toStdString()));
}

void EditorWidget::handleCut() {
  if (editor->get_selection().active) {
    auto rawText = editor->copy();
    QString text = QString::fromUtf8(rawText);

    if (!text.isEmpty()) {
      QApplication::clipboard()->setText(text);
    }

    applyChangeSet(editor->delete_forwards());
  }
}

void EditorWidget::handleRedo() { applyChangeSet(editor->redo()); }

void EditorWidget::handleUndo() { applyChangeSet(editor->undo()); }

void EditorWidget::resetFontSize() { setFontSize(DEFAULT_FONT_SIZE); }

void EditorWidget::increaseFontSize() {
  if (font.pointSizeF() < FONT_UPPER_LIMIT) {
    setFontSize(font.pointSizeF() + FONT_STEP);
  }
}

void EditorWidget::decreaseFontSize() {
  if (font.pointSizeF() > FONT_LOWER_LIMIT) {
    setFontSize(font.pointSizeF() - FONT_STEP);
  }
}

void EditorWidget::setFontSize(double newFontSize) {
  font.setPointSizeF(newFontSize);
  fontMetrics = QFontMetricsF(font);

  viewport()->update();
  UiUtils::setFontSize(configManager, neko::FontType::Editor, newFontSize);
  emit fontSizeChanged(newFontSize);
}

double EditorWidget::getTextWidth(const QString &text,
                                  double horizontalOffset) const {
  return fontMetrics.horizontalAdvance(text) - horizontalOffset;
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  if (editor == nullptr) {
    return;
  }

  QPainter painter(viewport());

  int lineCount = editor->get_line_count();

  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();

  int firstVisibleLine = verticalOffset / lineHeight;
  int visibleLineCount = viewportHeight / lineHeight;
  int lastVisibleLine = qMin(
      firstVisibleLine + visibleLineCount + EXTRA_VERTICAL_LINES, lineCount);

  ViewportContext ctx = {lineHeight, firstVisibleLine, lastVisibleLine,
                         verticalOffset, horizontalOffset};

  drawText(&painter, ctx);
  drawCursors(&painter, ctx);
  drawSelections(&painter, ctx);
}

void EditorWidget::drawText(QPainter *painter, const ViewportContext &ctx) {
  painter->setPen(TEXT_COLOR);
  painter->setFont(font);

  for (int line = ctx.firstVisibleLine; line <= ctx.lastVisibleLine; line++) {
    auto rawLine = editor->get_line(line);
    QString lineText = QString::fromUtf8(rawLine);

    auto actualY =
        (line * ctx.lineHeight) +
        (ctx.lineHeight + fontMetrics.ascent() - fontMetrics.descent()) / 2.0 -
        ctx.verticalOffset;

    painter->drawText(QPointF(-ctx.horizontalOffset, actualY), lineText);
  }
}

void EditorWidget::drawSelections(QPainter *painter,
                                  const ViewportContext &ctx) {
  if (!editor->get_selection().active) {
    return;
  }

  auto accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  QColor selectionColor = QColor(accentColor);
  selectionColor.setAlpha(50);
  painter->setBrush(selectionColor);
  painter->setPen(Qt::transparent);

  neko::Selection selection = editor->get_selection();
  int startRow = selection.start.row;
  int endRow = selection.end.row;
  int startCol = selection.start.col;
  int endCol = selection.end.col;

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
  auto rawLine = editor->get_line(startRow);
  QString text = QString::fromUtf8(rawLine);

  QString selection_text = text.mid(startCol, endCol - startCol);
  QString selection_before_text = text.mid(0, startCol);

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

  auto rawLine = editor->get_line(startRow);
  QString text = QString::fromUtf8(rawLine);

  if (text.isEmpty())
    text = " ";

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

    auto rawLine = editor->get_line(i);
    QString text = QString::fromUtf8(rawLine);

    if (text.isEmpty())
      text = " ";

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

  auto rawLine = editor->get_line(endRow);
  QString text = QString::fromUtf8(rawLine);
  QString selectionText = text.mid(0, endCol);

  double width = fontMetrics.horizontalAdvance(selectionText);

  painter->drawRect(getLineRect(endRow, -ctx.horizontalOffset,
                                width - ctx.horizontalOffset, ctx));
}

void EditorWidget::drawCursors(QPainter *painter, const ViewportContext &ctx) {
  auto cursors = editor->get_cursor_positions();

  for (auto cursor : cursors) {
    int cursorRow = cursor.row;
    int cursorCol = cursor.col;

    if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
      return;
    }

    // Draw line highlight
    painter->setPen(Qt::NoPen);
    painter->setBrush(QBrush(LINE_HIGHLIGHT_COLOR));
    painter->drawRect(getLineRect(
        cursorRow, 0, viewport()->width() + ctx.horizontalOffset, ctx));

    if (!hasFocus()) {
      return;
    }

    auto rawLine = editor->get_line(cursorRow);
    QString text = QString::fromUtf8(rawLine);
    QString textBeforeCursor = text.left(cursorCol);

    qreal cursorX = fontMetrics.horizontalAdvance(textBeforeCursor);

    if (cursorX < 0 || cursorX > viewport()->width() + ctx.horizontalOffset) {
      return;
    }

    auto accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
    painter->setPen(accentColor);
    painter->setBrush(Qt::NoBrush);

    double x = cursorX - ctx.horizontalOffset;
    painter->drawLine(QLineF(QPointF(x, getLineTopY(cursorRow, ctx)),
                             QPointF(x, getLineBottomY(cursorRow, ctx))));
  }
}
