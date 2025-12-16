#include "gutter_widget.h"

GutterWidget::GutterWidget(neko::Editor *editor,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), editor(editor), configManager(configManager),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, neko::FontType::Editor)),
      fontMetrics(font) {
  setFocusPolicy(Qt::NoFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  QString bgHex =
      UiUtils::getThemeColor(themeManager, "editor.gutter.background", "black");
  setStyleSheet(QString("GutterWidget { background: %1; }").arg(bgHex));

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->update(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->update(); });
}

GutterWidget::~GutterWidget() {}

void GutterWidget::setEditor(neko::Editor *newEditor) { editor = newEditor; }

void GutterWidget::onBufferChanged() { viewport()->update(); }

void GutterWidget::onCursorChanged() { viewport()->update(); }

void GutterWidget::onSelectionChanged() { viewport()->update(); }

void GutterWidget::onViewportChanged() { updateDimensionsAndRepaint(); }

QSize GutterWidget::sizeHint() const {
  return QSize(measureContent() + VIEWPORT_PADDING, height());
}

double GutterWidget::measureContent() const {
  if (editor == nullptr) {
    return 0;
  }

  int lineCount = editor->get_line_count();

  return fontMetrics.horizontalAdvance(QString::number(lineCount));
}

void GutterWidget::wheelEvent(QWheelEvent *event) {
  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;

  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  verticalScrollBar()->setValue(newVerticalScrollOffset);
  viewport()->update();
}

void GutterWidget::handleViewportUpdate() {
  if (editor == nullptr) {
    return;
  }

  int lineCount = editor->get_line_count();

  auto viewportHeight = (lineCount * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureContent();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);
}

void GutterWidget::updateDimensionsAndRepaint() {
  handleViewportUpdate();
  updateGeometry();
  viewport()->update();
}

void GutterWidget::onEditorLineCountChanged() { updateDimensionsAndRepaint(); }

void GutterWidget::onEditorCursorPositionChanged() { viewport()->update(); }

void GutterWidget::onEditorFontSizeChanged(qreal newSize) {
  font.setPointSizeF(newSize);
  fontMetrics = QFontMetricsF(font);
  updateDimensionsAndRepaint();
  updateGeometry();
}

void GutterWidget::paintEvent(QPaintEvent *event) {
  if (editor == nullptr) {
    return;
  }

  QPainter painter(viewport());

  int lineCount = editor->get_line_count();
  double lineHeight = fontMetrics.height();
  double verticalOffset = verticalScrollBar()->value();
  double viewportHeight = viewport()->height();
  double horizontalOffset = horizontalScrollBar()->value();
  int firstVisibleLine = verticalOffset / lineHeight;
  int visibleLineCount = viewportHeight / lineHeight;
  int lastVisibleLine =
      qMin(firstVisibleLine + visibleLineCount + 1, (int)lineCount - 1);
  ViewportContext ctx = {lineHeight, firstVisibleLine, lastVisibleLine,
                         verticalOffset, horizontalOffset};

  drawText(&painter, ctx, lineCount);
  drawLineHighlight(&painter, ctx);
}

void GutterWidget::drawText(QPainter *painter, const ViewportContext &ctx,
                            int lineCount) {
  painter->setPen(TEXT_COLOR);
  painter->setFont(font);

  auto verticalOffset = verticalScrollBar()->value();
  auto horizontalOffset = horizontalScrollBar()->value();
  double viewportWidth = viewport()->width();

  int maxLineNumber = lineCount;
  int maxLineWidth =
      fontMetrics.horizontalAdvance(QString::number(maxLineNumber));
  double numWidth = fontMetrics.horizontalAdvance(QString::number(1));

  neko::Selection selection = editor->get_selection();
  int selectionStartRow = selection.start.row;
  int selectionEndRow = selection.end.row;
  int selectionStartCol = selection.start.col;
  int selectionEndCol = selection.end.col;
  bool selectionActive = selection.active;

  for (int line = ctx.firstVisibleLine; line <= ctx.lastVisibleLine; ++line) {
    bool cursorIsOnLine = editor->cursor_exists_at_row(line);
    auto y =
        (line * fontMetrics.height()) +
        (fontMetrics.height() + fontMetrics.ascent() - fontMetrics.descent()) /
            2.0 -
        verticalOffset;

    QString lineNum = QString::number(line + 1);
    int lineNumWidth = fontMetrics.horizontalAdvance(lineNum);
    double x = (width() - maxLineWidth - numWidth) / 2.0 +
               (maxLineWidth - lineNumWidth) - horizontalOffset;

    if (cursorIsOnLine || (selectionActive && line >= selectionStartRow &&
                           line <= selectionEndRow)) {
      painter->setPen(CURRENT_LINE_COLOR);
    } else {
      painter->setPen(TEXT_COLOR);
    }

    painter->drawText(QPointF(x, y), lineNum);
  }
}

void GutterWidget::drawLineHighlight(QPainter *painter,
                                     const ViewportContext &ctx) {
  auto cursors = editor->get_cursor_positions();

  std::vector<int> highlightedLines = std::vector<int>();
  for (auto cursor : cursors) {
    int cursorRow = cursor.row;
    int cursorCol = cursor.col;

    if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
      return;
    }

    if (std::find(highlightedLines.begin(), highlightedLines.end(),
                  cursorRow) != highlightedLines.end()) {
      continue;
    }

    highlightedLines.push_back(cursorRow);

    // Draw line highlight
    auto lineHighlightColor =
        UiUtils::getThemeColor(themeManager, "editor.highlight");
    painter->setPen(Qt::NoPen);
    painter->setBrush(lineHighlightColor);
    painter->drawRect(getLineRect(cursorRow, 0, viewport()->width(), ctx));
  }
}
