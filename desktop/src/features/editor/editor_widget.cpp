#include "editor_widget.h"
#include "utils/gui_utils.h"

EditorWidget::EditorWidget(neko::Editor *editor,
                           EditorController *editorController,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), editor(editor), editorController(editorController),
      renderer(new EditorRenderer()), configManager(configManager),
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
          &EditorWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &EditorWidget::redraw);
}

EditorWidget::~EditorWidget() {}

void EditorWidget::setEditor(neko::Editor *newEditor) { editor = newEditor; }

double EditorWidget::measureWidth() {
  if (!editor) {
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

void EditorWidget::updateDimensions() {
  if (!editor) {
    return;
  }

  int lineCount = editor->get_line_count();

  auto viewportHeight = (lineCount * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureWidth();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  bool horizontalScrollBarVisible = horizontalScrollBar()->isVisible();
  double horizontalScrollBarHeight = horizontalScrollBar()->height();
  double adjustedVerticalRange =
      viewportHeight -
      (horizontalScrollBarVisible ? horizontalScrollBarHeight : 0.0);

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, adjustedVerticalRange);
  redraw();
}

void EditorWidget::redraw() { viewport()->update(); }

void EditorWidget::mousePressEvent(QMouseEvent *event) {
  if (!editor) {
    return;
  }

  RowCol rc = convertMousePositionToRowCol(event->pos().x(), event->pos().y());

  if (event->modifiers().testFlag(Qt::AltModifier)) {
    if (editor->cursor_exists_at(rc.row, rc.col)) {
      editor->remove_cursor(rc.row, rc.col);
    } else {
      editorController->addCursor(neko::AddCursorDirectionKind::At, rc.row,
                                  rc.col);
    }
  } else {
    editor->move_to(rc.row, rc.col, true);
  }

  auto numberOfCursors = editor->get_cursor_positions().size();
  emit cursorPositionChanged(rc.row, rc.col, numberOfCursors);

  redraw();
}

void EditorWidget::mouseMoveEvent(QMouseEvent *event) {
  if (!editor) {
    return;
  }

  if (event->buttons() == Qt::LeftButton) {
    RowCol rc =
        convertMousePositionToRowCol(event->pos().x(), event->pos().y());

    editor->select_to(rc.row, rc.col);

    auto numberOfCursors = editor->get_cursor_positions().size();
    emit cursorPositionChanged(rc.row, rc.col, numberOfCursors);

    redraw();
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
  redraw();
}

bool EditorWidget::focusNextPrevChild(bool next) { return false; }

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  if (!editor) {
    return;
  }

  const auto mods = event->modifiers();
  const bool ctrl = mods.testFlag(Qt::ControlModifier);
  const bool shift = mods.testFlag(Qt::ShiftModifier);
  const bool meta = mods.testFlag(Qt::MetaModifier);

  if (ctrl) {
    switch (event->key()) {
    case Qt::Key_A:
      editorController->selectAll();
      return;
    case Qt::Key_C:
      editorController->copy();
      return;
    case Qt::Key_V: {
      editorController->paste();
      return;
    }
    case Qt::Key_X:
      editorController->cut();
      return;

    case Qt::Key_P:
      if (meta) {
        editorController->addCursor(neko::AddCursorDirectionKind::Above);
        return;
      }
    case Qt::Key_N:
      if (meta) {
        editorController->addCursor(neko::AddCursorDirectionKind::Below);
        return;
      }

    case Qt::Key_Z:
      if (shift) {
        editorController->redo();
      } else {
        editorController->undo();
      }
      return;

    case Qt::Key_Equal:
      increaseFontSize();
      return;
    case Qt::Key_Minus:
      decreaseFontSize();
      return;
    case Qt::Key_0:
      resetFontSize();
      return;
    }
  }

  switch (event->key()) {
  case Qt::Key_Left:
    editorController->moveOrSelectLeft(shift);
    break;
  case Qt::Key_Right:
    editorController->moveOrSelectRight(shift);
    break;
  case Qt::Key_Up:
    editorController->moveOrSelectUp(shift);
    break;
  case Qt::Key_Down:
    editorController->moveOrSelectDown(shift);
    break;
  case Qt::Key_Enter:
  case Qt::Key_Return:
    editorController->insertNewline();
    break;
  case Qt::Key_Backspace:
    editorController->backspace();
    break;
  case Qt::Key_Delete:
    editorController->deleteForwards();
    break;
  case Qt::Key_Tab:
    editorController->insertTab();
    break;
  case Qt::Key_Escape:
    editorController->clearSelectionOrCursors();
    break;
  default:
    editorController->insertText(event->text().toStdString());
    break;
  }
}

void EditorWidget::onBufferChanged() { redraw(); }

void EditorWidget::onCursorChanged() {
  scrollToCursor();
  redraw();
}

void EditorWidget::onSelectionChanged() { redraw(); }

void EditorWidget::onViewportChanged() { updateDimensions(); }

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

  UiUtils::setFontSize(configManager, neko::FontType::Editor, newFontSize);
  emit fontSizeChanged(newFontSize);

  updateDimensions();
}

double EditorWidget::getTextWidth(const QString &text,
                                  double horizontalOffset) const {
  return fontMetrics.horizontalAdvance(text) - horizontalOffset;
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  if (!editor) {
    return;
  }

  QPainter painter(viewport());

  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();

  int lineCount = editor->get_line_count();
  int firstVisibleLine = verticalOffset / lineHeight;
  int visibleLineCount = viewportHeight / lineHeight;
  int lastVisibleLine =
      qMin(firstVisibleLine + visibleLineCount + EXTRA_VERTICAL_LINES,
           lineCount - 1);

  ViewportContext ctx = {lineHeight,     firstVisibleLine, lastVisibleLine,
                         verticalOffset, horizontalOffset, viewportWidth,
                         viewportHeight};

  rust::Vec<rust::String> rawLines = editor->get_lines();
  rust::Vec<neko::CursorPosition> cursors = editor->get_cursor_positions();
  neko::Selection selections = editor->get_selection();

  double fontAscent = fontMetrics.ascent();
  double fontDescent = fontMetrics.descent();
  bool hasFocus = this->hasFocus();

  QString textColor = UiUtils::getThemeColor(themeManager, "editor.foreground");
  QString accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  QString highlightColor =
      UiUtils::getThemeColor(themeManager, "editor.highlight");

  RenderTheme theme = {textColor, accentColor, highlightColor};

  auto measureWidth = [this](const QString &s) {
    return fontMetrics.horizontalAdvance(s);
  };
  RenderState state = {
      rawLines,       cursors,          selections,  theme,      lineCount,
      verticalOffset, horizontalOffset, lineHeight,  fontAscent, fontDescent,
      font,           hasFocus,         measureWidth};

  renderer->paint(painter, state, ctx);
}
