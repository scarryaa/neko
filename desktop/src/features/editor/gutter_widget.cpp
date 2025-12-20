#include "gutter_widget.h"
#include "features/editor/controllers/editor_controller.h"

GutterWidget::GutterWidget(EditorController *editorController,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), editorController(editorController),
      configManager(configManager), renderer(new GutterRenderer()),
      themeManager(themeManager),
      font(UiUtils::loadFont(configManager, neko::FontType::Editor)),
      fontMetrics(font) {
  setFocusPolicy(Qt::NoFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setFrameShape(QFrame::NoFrame);
  setAutoFillBackground(false);

  applyTheme();

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          &GutterWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &GutterWidget::redraw);

  // EditorController -> GutterWidget connections
  connect(editorController, &EditorController::lineCountChanged, this,
          &GutterWidget::onEditorLineCountChanged);
  connect(editorController, &EditorController::bufferChanged, this,
          &GutterWidget::onBufferChanged);
  connect(editorController, &EditorController::cursorChanged, this,
          &GutterWidget::onCursorChanged);
  connect(editorController, &EditorController::selectionChanged, this,
          &GutterWidget::onSelectionChanged);
  connect(editorController, &EditorController::viewportChanged, this,
          &GutterWidget::onViewportChanged);
}

GutterWidget::~GutterWidget() {}

void GutterWidget::applyTheme() {
  QString bgHex =
      UiUtils::getThemeColor(themeManager, "editor.gutter.background", "black");

  setStyleSheet(QString("GutterWidget { background: %1; }").arg(bgHex));
  redraw();
}

void GutterWidget::redraw() { viewport()->update(); }

void GutterWidget::onBufferChanged() { redraw(); }

void GutterWidget::onCursorChanged() { redraw(); }

void GutterWidget::onSelectionChanged() { redraw(); }

void GutterWidget::onViewportChanged() { updateDimensions(); }

QSize GutterWidget::sizeHint() const {
  return QSize(measureWidth() + VIEWPORT_PADDING, height());
}

double GutterWidget::measureWidth() const {
  if (!editorController) {
    return 0;
  }

  int lineCount = editorController->getLineCount();

  return fontMetrics.horizontalAdvance(QString::number(lineCount));
}

void GutterWidget::wheelEvent(QWheelEvent *event) {
  auto verticalScrollOffset = verticalScrollBar()->value();
  double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  verticalScrollBar()->setValue(newVerticalScrollOffset);
  redraw();
}

void GutterWidget::updateDimensions() {
  if (!editorController) {
    return;
  }

  int lineCount = editorController->getLineCount();
  auto viewportHeight = (lineCount * fontMetrics.height()) -
                        viewport()->height() + VIEWPORT_PADDING;
  auto contentWidth = measureWidth();
  auto viewportWidth = contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);

  updateGeometry();
  redraw();
}

void GutterWidget::onEditorLineCountChanged() { updateDimensions(); }

void GutterWidget::onEditorCursorPositionChanged() { redraw(); }

void GutterWidget::onEditorFontSizeChanged(qreal newSize) {
  font.setPointSizeF(newSize);
  fontMetrics = QFontMetricsF(font);

  updateDimensions();
}

void GutterWidget::paintEvent(QPaintEvent *event) {
  if (!editorController) {
    return;
  }

  QPainter painter(viewport());

  double verticalOffset = verticalScrollBar()->value();
  double horizontalOffset = horizontalScrollBar()->value();
  double viewportHeight = viewport()->height();
  double viewportWidth = viewport()->width();
  double lineHeight = fontMetrics.height();

  int lineCount = editorController->getLineCount();
  int firstVisibleLine =
      qMax(0, qMin((int)(verticalOffset / lineHeight), lineCount - 1));
  int visibleLineCount = viewportHeight / lineHeight;
  int lastVisibleLine =
      qMax(0, qMin(firstVisibleLine + visibleLineCount + EXTRA_VERTICAL_LINES,
                   lineCount - 1));

  ViewportContext ctx = {lineHeight,     firstVisibleLine, lastVisibleLine,
                         verticalOffset, horizontalOffset, viewportWidth,
                         viewportHeight};

  std::vector<neko::CursorPosition> cursors =
      editorController->getCursorPositions();
  neko::Selection selections = editorController->getSelection();
  bool isEmpty = editorController->bufferIsEmpty();

  double fontAscent = fontMetrics.ascent();
  double fontDescent = fontMetrics.descent();
  bool hasFocus = this->hasFocus();

  QString textColor =
      UiUtils::getThemeColor(themeManager, "editor.gutter.foreground");
  QString accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  QString highlightColor =
      UiUtils::getThemeColor(themeManager, "editor.highlight");
  QString activeLineTextColor =
      UiUtils::getThemeColor(themeManager, "editor.gutter.foreground.active");

  RenderTheme theme = {textColor, activeLineTextColor, accentColor,
                       highlightColor};

  auto measureWidth = [this](const QString &s) {
    return fontMetrics.horizontalAdvance(s);
  };
  RenderState state = {
      QStringList(),  cursors,          selections, theme,       lineCount,
      verticalOffset, horizontalOffset, lineHeight, fontAscent,  fontDescent,
      font,           hasFocus,         isEmpty,    measureWidth};

  renderer->paint(painter, state, ctx);
}
