#include "gutter_widget.h"
#include "features/editor/render/render_state.h"
#include "utils/gui_utils.h"

GutterWidget::GutterWidget(EditorController *editorController,
                           neko::ConfigManager &configManager,
                           const GutterTheme &theme, QWidget *parent)
    : QScrollArea(parent), editorController(editorController),
      configManager(configManager), renderer(new GutterRenderer()),
      theme(theme),
      font(UiUtils::loadFont(configManager, neko::FontType::Editor)),
      fontMetrics(font) {
  setFocusPolicy(Qt::NoFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  setAndApplyTheme(theme);

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          &GutterWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &GutterWidget::redraw);
}

void GutterWidget::redraw() const { viewport()->update(); }

void GutterWidget::setAndApplyTheme(const GutterTheme &newTheme) {
  theme = newTheme;

  setStyleSheet(
      QString("GutterWidget { background: %1; }").arg(theme.backgroundColor));

  redraw();
}

void GutterWidget::updateDimensions() {
  if (editorController == nullptr) {
    return;
  }

  const int lineCount = editorController->getLineCount();
  const auto viewportHeight = (lineCount * fontMetrics.height()) -
                              viewport()->height() + VIEWPORT_PADDING;
  const auto contentWidth = measureWidth();
  const auto viewportWidth =
      contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, static_cast<int>(viewportWidth));
  verticalScrollBar()->setRange(0, static_cast<int>(viewportHeight));

  updateGeometry();
  redraw();
}

void GutterWidget::setEditorController(EditorController *newEditorController) {
  editorController = newEditorController;
}

QSize GutterWidget::sizeHint() const {
  return {static_cast<int>(measureWidth() + VIEWPORT_PADDING), height()};
}

void GutterWidget::paintEvent(QPaintEvent *event) {
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

  const std::vector<neko::CursorPosition> cursors =
      editorController->getCursorPositions();
  const neko::Selection selections = editorController->getSelection();
  const bool isEmpty = editorController->bufferIsEmpty();

  const double fontAscent = fontMetrics.ascent();
  const double fontDescent = fontMetrics.descent();
  const bool hasFocus = this->hasFocus();

  const QString textColor = theme.foregroundColor;
  const QString accentColor = theme.accentColor;
  const QString highlightColor = theme.highlightColor;
  const QString activeLineTextColor = theme.foregroundActiveColor;

  const RenderTheme theme = {textColor, activeLineTextColor, accentColor,
                             highlightColor};

  const auto measureWidth = [this](const QString &string) {
    return fontMetrics.horizontalAdvance(string);
  };
  const RenderState state = {
      QStringList(),  cursors,          selections, theme,       lineCount,
      verticalOffset, horizontalOffset, lineHeight, fontAscent,  fontDescent,
      font,           hasFocus,         isEmpty,    measureWidth};

  GutterRenderer::paint(painter, state, ctx);
}

void GutterWidget::wheelEvent(QWheelEvent *event) {
  const auto verticalScrollOffset = verticalScrollBar()->value();
  const double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  const auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  verticalScrollBar()->setValue(static_cast<int>(newVerticalScrollOffset));
  redraw();
}

void GutterWidget::onEditorFontSizeChanged(const qreal newSize) {
  font.setPointSizeF(newSize);
  fontMetrics = QFontMetricsF(font);

  updateDimensions();
}

void GutterWidget::onEditorLineCountChanged() { updateDimensions(); }

void GutterWidget::onEditorCursorPositionChanged() const { redraw(); }

void GutterWidget::onBufferChanged() const { redraw(); }

void GutterWidget::onCursorChanged() const { redraw(); }

void GutterWidget::onSelectionChanged() const { redraw(); }

void GutterWidget::onViewportChanged() { updateDimensions(); }

double GutterWidget::measureWidth() const {
  if (editorController == nullptr) {
    return 0;
  }

  const int lineCount = editorController->getLineCount();

  return fontMetrics.horizontalAdvance(QString::number(lineCount));
}
