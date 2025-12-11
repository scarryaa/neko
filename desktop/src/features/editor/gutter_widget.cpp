#include "gutter_widget.h"

GutterWidget::GutterWidget(NekoEditor *editor, NekoConfigManager *configManager,
                           NekoThemeManager *themeManager, QWidget *parent)
    : QScrollArea(parent), editor(editor), configManager(configManager),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, UiUtils::FontType::Editor)),
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
          [this]() { viewport()->repaint(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
}

GutterWidget::~GutterWidget() {}

void GutterWidget::setEditor(NekoEditor *newEditor) { editor = newEditor; }

QSize GutterWidget::sizeHint() const {
  return QSize(measureContent() + VIEWPORT_PADDING, height());
}

double GutterWidget::measureContent() const {
  if (editor == nullptr)
    return 0;

  size_t lineCount = 0;
  neko_editor_get_line_count(editor, &lineCount);

  return fontMetrics.horizontalAdvance(QString::number(lineCount));
}

void GutterWidget::wheelEvent(QWheelEvent *event) {
  if (editor == nullptr)
    return;

  auto horizontalScrollOffset = horizontalScrollBar()->value();
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;

  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  verticalScrollBar()->setValue(newVerticalScrollOffset);
  viewport()->repaint();
}

void GutterWidget::handleViewportUpdate() {
  if (editor == nullptr)
    return;

  size_t line_count;
  neko_editor_get_line_count(editor, &line_count);

  auto viewportHeight = (line_count * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureContent();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);
}

void GutterWidget::updateDimensionsAndRepaint() {
  handleViewportUpdate();
  viewport()->repaint();
}

void GutterWidget::onEditorLineCountChanged() { updateDimensionsAndRepaint(); }

void GutterWidget::onEditorCursorPositionChanged() { viewport()->repaint(); }

void GutterWidget::onEditorFontSizeChanged(qreal newSize) {
  font.setPointSizeF(newSize);
  fontMetrics = QFontMetricsF(font);
  updateDimensionsAndRepaint();
  updateGeometry();
}

void GutterWidget::paintEvent(QPaintEvent *event) {
  if (editor == nullptr)
    return;

  QPainter painter(viewport());

  size_t lineCount;
  neko_editor_get_line_count(editor, &lineCount);

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

  size_t cursorRow, cursorCol;
  neko_editor_get_cursor_position(editor, &cursorRow, &cursorCol);

  size_t selectionStartRow, selectionStartCol, selectionEndRow, selectionEndCol;
  neko_editor_get_selection_start(editor, &selectionStartRow,
                                  &selectionStartCol);
  neko_editor_get_selection_end(editor, &selectionEndRow, &selectionEndCol);
  bool selectionActive = neko_editor_get_selection_active(editor);

  for (int line = ctx.firstVisibleLine; line <= ctx.lastVisibleLine; ++line) {
    auto y =
        (line * fontMetrics.height()) +
        (fontMetrics.height() + fontMetrics.ascent() - fontMetrics.descent()) /
            2.0 -
        verticalOffset;

    QString lineNum = QString::number(line + 1);
    int lineNumWidth = fontMetrics.horizontalAdvance(lineNum);
    double x = (width() - maxLineWidth - numWidth) / 2.0 +
               (maxLineWidth - lineNumWidth) - horizontalOffset;

    if (line == cursorRow || (selectionActive && line >= selectionStartRow &&
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
  size_t cursorRow, cursorCol;
  neko_editor_get_cursor_position(editor, &cursorRow, &cursorCol);

  if (cursorRow < ctx.firstVisibleLine || cursorRow > ctx.lastVisibleLine) {
    return;
  }

  // Draw line highlight
  painter->setPen(Qt::NoPen);
  painter->setBrush(QBrush(LINE_HIGHLIGHT_COLOR));
  painter->drawRect(getLineRect(cursorRow, 0, viewport()->width(), ctx));
}
