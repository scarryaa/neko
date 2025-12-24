#include "editor_widget.h"
#include "utils/editor_utils.h"
#include "utils/gui_utils.h"

EditorWidget::EditorWidget(EditorController *editorController,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), editorController(editorController),
      renderer(new EditorRenderer()), configManager(configManager),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, neko::FontType::Editor)),
      fontMetrics(font) {
  setFocusPolicy(Qt::StrongFocus);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  tripleArmTimer.setSingleShot(true);
  suppressDblTimer.setSingleShot(true);

  connect(&tripleArmTimer, &QTimer::timeout, this,
          [this] { tripleArmed = false; });
  connect(&suppressDblTimer, &QTimer::timeout, this,
          [this] { suppressNextDouble = false; });

  applyTheme();

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          &EditorWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &EditorWidget::redraw);
}

void EditorWidget::applyTheme() {
  const QString bgHex =
      UiUtils::getThemeColor(themeManager, "editor.background", "#000000");

  setStyleSheet(
      UiUtils::getScrollBarStylesheet(themeManager, "EditorWidget", bgHex));
  redraw();
}

void EditorWidget::redraw() const { viewport()->update(); }

void EditorWidget::updateDimensions() {
  if (editorController == nullptr) {
    return;
  }

  const int lineCount = editorController->getLineCount();
  const auto viewportHeight = (lineCount * fontMetrics.height()) -
                              viewport()->height() + VIEWPORT_PADDING;
  const auto contentWidth = measureWidth();
  const auto viewportWidth =
      contentWidth - viewport()->width() + VIEWPORT_PADDING;

  const bool horizontalScrollBarVisible = horizontalScrollBar()->isVisible();
  const double horizontalScrollBarHeight = horizontalScrollBar()->height();
  const double adjustedVerticalRange =
      viewportHeight -
      (horizontalScrollBarVisible ? horizontalScrollBarHeight : 0.0);

  horizontalScrollBar()->setRange(0, static_cast<int>(viewportWidth));
  verticalScrollBar()->setRange(0, static_cast<int>(adjustedVerticalRange));
  redraw();
}

void EditorWidget::setEditorController(EditorController *newEditorController) {
  editorController = newEditorController;
}

void EditorWidget::onBufferChanged() const { redraw(); }

void EditorWidget::onCursorChanged() {
  scrollToCursor();
  redraw();
}

void EditorWidget::onSelectionChanged() const { redraw(); }

void EditorWidget::onViewportChanged() { updateDimensions(); }

static int tripleWindowMs() {
  static constexpr int TRIPLE_CLICK_MS = 120;
  return std::max(TRIPLE_CLICK_MS, QApplication::doubleClickInterval() / 2);
}

static bool nearPos(const QPoint &pointA, const QPoint &pointB,
                    const int pixelThreshold = 4) {
  return (pointA - pointB).manhattanLength() <= pixelThreshold;
}

void EditorWidget::keyPressEvent(QKeyEvent *event) {
  if (editorController == nullptr) {
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
    default:
      break;
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

void EditorWidget::mousePressEvent(QMouseEvent *event) {
  if (editorController == nullptr) {
    return;
  }

  const RowCol rowCol =
      convertMousePositionToRowCol(event->pos().x(), event->pos().y());

  if (event->modifiers().testFlag(Qt::AltModifier)) {
    tripleArmed = false;
    tripleArmTimer.stop();

    if (editorController->cursorExistsAt(rowCol.row, rowCol.col)) {
      editorController->removeCursor(rowCol.row, rowCol.col);
    } else {
      editorController->addCursor(neko::AddCursorDirectionKind::At, rowCol.row,
                                  rowCol.col);
    }

    redraw();

    event->accept();

    return;
  }

  if (event->button() == Qt::LeftButton) {
    if (tripleArmed && tripleArmTimer.isActive() &&
        nearPos(event->pos(), triplePos)) {
      tripleArmed = false;
      tripleArmTimer.stop();

      editorController->selectLine(tripleRow);

      const int lineCount = editorController->getLineCount();

      if (lineCount > 0) {
        lineSelectMode = true;
        wordSelectMode = false;
        lineAnchorRow = std::clamp(tripleRow, 0, lineCount - 1);
      } else {
        lineSelectMode = false;
      }

      redraw();

      event->accept();

      suppressNextDouble = true;
      suppressDblPos = event->pos();
      suppressDblTimer.start(QApplication::doubleClickInterval());

      return;
    }

    tripleArmed = false;
    tripleArmTimer.stop();

    wordSelectMode = false;
    lineSelectMode = false;
    editorController->moveTo(rowCol.row, rowCol.col, true);

    redraw();

    event->accept();
    return;
  }

  QAbstractScrollArea::mousePressEvent(event);
}

void EditorWidget::mouseDoubleClickEvent(QMouseEvent *event) {
  if (editorController == nullptr) {
    return;
  }

  if (suppressNextDouble && suppressDblTimer.isActive() &&
      nearPos(event->pos(), suppressDblPos)) {
    suppressNextDouble = false;
    suppressDblTimer.stop();
    tripleArmTimer.stop();

    const RowCol rowCol =
        convertMousePositionToRowCol(event->pos().x(), event->pos().y());

    editorController->moveTo(rowCol.row, rowCol.col, true);

    redraw();

    event->accept();
    return;
  }

  const RowCol rowCol =
      convertMousePositionToRowCol(event->pos().x(), event->pos().y());
  editorController->selectWord(rowCol.row, rowCol.col);

  const auto selection = editorController->getSelection();

  if (selection.active) {
    wordSelectMode = true;
    lineSelectMode = false;
    wordAnchorStart = {static_cast<int>(selection.start.row),
                       static_cast<int>(selection.start.col)};
    wordAnchorEnd = {static_cast<int>(selection.end.row),
                     static_cast<int>(selection.end.col)};
  } else {
    wordSelectMode = false;
    lineSelectMode = false;
  }

  tripleArmed = true;
  triplePos = event->pos();
  tripleRow = rowCol.row;
  tripleArmTimer.start(tripleWindowMs());

  redraw();
  event->accept();
}

void EditorWidget::mouseMoveEvent(QMouseEvent *event) {
  if (editorController == nullptr) {
    return;
  }

  if (wordSelectMode) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
      return;
    }

    const RowCol rowCol =
        convertMousePositionToRowCol(event->pos().x(), event->pos().y());
    editorController->selectWordDrag(wordAnchorStart.row, wordAnchorStart.col,
                                     wordAnchorEnd.row, wordAnchorEnd.col,
                                     rowCol.row, rowCol.col);

    redraw();
  } else if (lineSelectMode) {
    if (!event->buttons().testFlag(Qt::LeftButton)) {
      return;
    }

    const RowCol rowCol =
        convertMousePositionToRowCol(event->pos().x(), event->pos().y());
    editorController->selectLineDrag(lineAnchorRow, rowCol.row);

    redraw();
  } else {
    if (event->buttons() == Qt::LeftButton) {
      const RowCol rowCol =
          convertMousePositionToRowCol(event->pos().x(), event->pos().y());
      editorController->selectTo(rowCol.row, rowCol.col);

      redraw();
    }
  }
}

void EditorWidget::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    wordSelectMode = false;
    lineSelectMode = false;
  }

  QAbstractScrollArea::mouseReleaseEvent(event);
}

void EditorWidget::paintEvent(QPaintEvent *event) {
  if (editorController == nullptr) {
    return;
  }

  QPainter painter(viewport());

  const double verticalOffset = verticalScrollBar()->value();
  const double horizontalOffset = horizontalScrollBar()->value();
  const double viewportHeight = viewport()->height();
  const double viewportWidth = viewport()->width();
  const double lineHeight = fontMetrics.height();

  const int lineCount = editorController->getLineCount();
  const int firstVisibleLine = qMax(
      0, qMin(static_cast<int>(verticalOffset / lineHeight), lineCount - 1));
  const int visibleLineCount = static_cast<int>(viewportHeight / lineHeight);
  const int lastVisibleLine =
      qMax(0, qMin(firstVisibleLine + visibleLineCount + EXTRA_VERTICAL_LINES,
                   lineCount - 1));

  const ViewportContext ctx = {
      lineHeight,       firstVisibleLine, lastVisibleLine, verticalOffset,
      horizontalOffset, viewportWidth,    viewportHeight};

  const QStringList lines = editorController->getLines();
  const std::vector<neko::CursorPosition> cursors =
      editorController->getCursorPositions();
  const neko::Selection selections = editorController->getSelection();
  const bool isEmpty = editorController->isEmpty();

  const double fontAscent = fontMetrics.ascent();
  const double fontDescent = fontMetrics.descent();
  const bool hasFocus = this->hasFocus();

  const QString textColor =
      UiUtils::getThemeColor(themeManager, "editor.foreground");
  const QString accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  const QString highlightColor =
      UiUtils::getThemeColor(themeManager, "editor.highlight");

  const RenderTheme theme = {textColor, textColor, accentColor, highlightColor};

  const auto measureWidth = [this](const QString &string) {
    return fontMetrics.horizontalAdvance(string);
  };
  const RenderState state = {
      lines,          cursors,          selections, theme,       lineCount,
      verticalOffset, horizontalOffset, lineHeight, fontAscent,  fontDescent,
      font,           hasFocus,         isEmpty,    measureWidth};

  EditorRenderer::paint(painter, state, ctx);
}

void EditorWidget::wheelEvent(QWheelEvent *event) {
  const auto horizontalScrollOffset = horizontalScrollBar()->value();
  const auto verticalScrollOffset = verticalScrollBar()->value();
  const double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  const double horizontallDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().x() / 4.0;

  const auto newHorizontalScrollOffset =
      horizontalScrollOffset + horizontallDelta;
  const auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  horizontalScrollBar()->setValue(static_cast<int>(newHorizontalScrollOffset));
  verticalScrollBar()->setValue(static_cast<int>(newVerticalScrollOffset));
  redraw();
}

bool EditorWidget::focusNextPrevChild(bool next) { return false; }

double EditorWidget::getTextWidth(const QString &text,
                                  const double horizontalOffset) const {
  return fontMetrics.horizontalAdvance(text) - horizontalOffset;
}

static int xToCursorIndex(const QString &line, const QFont &font,
                          const qreal xPos) {
  static constexpr int MAX_TEXT_LINE_WIDTH = 1e9;

  QTextLayout layout(line, font);
  layout.beginLayout();

  QTextLine textLine = layout.createLine();

  if (!textLine.isValid()) {
    return 0;
  }

  textLine.setLineWidth(MAX_TEXT_LINE_WIDTH);

  layout.endLayout();

  return textLine.xToCursor(xPos);
}

RowCol EditorWidget::convertMousePositionToRowCol(const double xPos,
                                                  const double yPos) {
  const double lineHeight = fontMetrics.height();
  const int scrollX = horizontalScrollBar()->value();
  const int scrollY = verticalScrollBar()->value();

  const int lineCount = editorController->getLineCount();
  if (lineCount <= 0) {
    return {0, 0};
  }

  const int targetRow = static_cast<int>((yPos + scrollY) / lineHeight);
  const int lastRow = (lineCount - 1);
  const int row = std::min(targetRow, lastRow);

  const QString line = editorController->getLine(row);
  const auto targetX = static_cast<qreal>(xPos + scrollX);

  int col = xToCursorIndex(line, font, targetX);
  col = std::clamp(col, 0, static_cast<int>(line.length()));

  return {static_cast<int>(row), col};
}

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

void EditorWidget::resetFontSize() { setFontSize(DEFAULT_FONT_SIZE); }

void EditorWidget::setFontSize(double newFontSize) {
  font.setPointSizeF(newFontSize);
  fontMetrics = QFontMetricsF(font);

  UiUtils::setFontSize(configManager, neko::FontType::Editor, newFontSize);
  emit fontSizeChanged(newFontSize);

  updateDimensions();
}

void EditorWidget::scrollToCursor() {
  if (editorController == nullptr) {
    return;
  }

  const auto cursors = editorController->getCursorPositions();
  const auto cursor = cursors.at(0);

  const int targetRow = static_cast<int>(cursor.row);
  const double lineHeight = fontMetrics.height();

  const QString line = editorController->getLine(static_cast<int>(cursor.row));
  const QString textBeforeCursor = line.mid(0, static_cast<int>(cursor.col));

  const double viewportWidth = viewport()->width();
  const double viewportHeight = viewport()->height();
  const double horizontalScrollOffset = horizontalScrollBar()->value();
  const double verticalScrollOffset = verticalScrollBar()->value();

  const double targetY = targetRow * lineHeight;
  const double targetYBottom = targetY + lineHeight;
  const double targetX = fontMetrics.horizontalAdvance(textBeforeCursor);

  if (targetX > viewportWidth - VIEWPORT_PADDING + horizontalScrollOffset) {
    horizontalScrollBar()->setValue(
        static_cast<int>(targetX - viewportWidth + VIEWPORT_PADDING));
  } else if (targetX < horizontalScrollOffset + VIEWPORT_PADDING) {
    horizontalScrollBar()->setValue(
        static_cast<int>(targetX - VIEWPORT_PADDING));
  }

  if (targetYBottom >
      viewportHeight - VIEWPORT_PADDING + verticalScrollOffset) {
    verticalScrollBar()->setValue(
        static_cast<int>(targetYBottom - viewportHeight + VIEWPORT_PADDING));
  } else if (targetY < verticalScrollOffset + VIEWPORT_PADDING) {
    verticalScrollBar()->setValue(static_cast<int>(targetY - VIEWPORT_PADDING));
  }
}

double EditorWidget::measureWidth() const {
  if (editorController == nullptr) {
    return 0;
  }

  const int lineCount = editorController->getLineCount();

  for (int i = 0; i < lineCount; i++) {
    if (editorController->needsWidthMeasurement(i)) {
      const QString line = editorController->getLine(i);
      const double width = fontMetrics.horizontalAdvance(line);
      editorController->setLineWidth(i, width);
    }
  }

  return editorController->getMaxWidth();
}
