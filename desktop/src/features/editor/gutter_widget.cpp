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
  const QString bgHex =
      UiUtils::getThemeColor(themeManager, "editor.gutter.background", "black");

  setStyleSheet(QString("GutterWidget { background: %1; }").arg(bgHex));
  redraw();
}

void GutterWidget::redraw() const { viewport()->update(); }

void GutterWidget::onBufferChanged() { redraw(); }

void GutterWidget::onCursorChanged() { redraw(); }

void GutterWidget::onSelectionChanged() { redraw(); }

void GutterWidget::onViewportChanged() { updateDimensions(); }

QSize GutterWidget::sizeHint() const {
  return QSize(measureWidth() + VIEWPORT_PADDING, height());
}

void GutterWidget::setEditorController(EditorController *newEditorController) {
  editorController = newEditorController;
}

const double GutterWidget::measureWidth() const {
  if (!editorController) {
    return 0;
  }

  const int lineCount = editorController->getLineCount();

  return fontMetrics.horizontalAdvance(QString::number(lineCount));
}

void GutterWidget::wheelEvent(QWheelEvent *event) {
  const auto verticalScrollOffset = verticalScrollBar()->value();
  const double verticalDelta =
      (event->isInverted() ? -1 : 1) * event->angleDelta().y() / 4.0;
  const auto newVerticalScrollOffset = verticalScrollOffset + verticalDelta;

  verticalScrollBar()->setValue(newVerticalScrollOffset);
  redraw();
}

void GutterWidget::updateDimensions() {
  if (!editorController) {
    return;
  }

  const int lineCount = editorController->getLineCount();
  const auto viewportHeight = (lineCount * fontMetrics.height()) -
                              viewport()->height() + VIEWPORT_PADDING;
  const auto contentWidth = measureWidth();
  const auto viewportWidth =
      contentWidth - viewport()->width() + VIEWPORT_PADDING;

  horizontalScrollBar()->setRange(0, viewportWidth);
  verticalScrollBar()->setRange(0, viewportHeight);

  updateGeometry();
  redraw();
}

void GutterWidget::onEditorLineCountChanged() { updateDimensions(); }

void GutterWidget::onEditorCursorPositionChanged() { redraw(); }

void GutterWidget::onEditorFontSizeChanged(const qreal newSize) {
  font.setPointSizeF(newSize);
  fontMetrics = QFontMetricsF(font);

  updateDimensions();
}

void GutterWidget::paintEvent(QPaintEvent *event) {
  if (!editorController) {
    return;
  }

  QPainter painter(viewport());

  const double verticalOffset = verticalScrollBar()->value();
  const double horizontalOffset = horizontalScrollBar()->value();
  const double viewportHeight = viewport()->height();
  const double viewportWidth = viewport()->width();
  const double lineHeight = fontMetrics.height();

  const int lineCount = editorController->getLineCount();
  const int firstVisibleLine =
      qMax(0, qMin((int)(verticalOffset / lineHeight), lineCount - 1));
  const int visibleLineCount = viewportHeight / lineHeight;
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

  const QString textColor =
      UiUtils::getThemeColor(themeManager, "editor.gutter.foreground");
  const QString accentColor = UiUtils::getThemeColor(themeManager, "ui.accent");
  const QString highlightColor =
      UiUtils::getThemeColor(themeManager, "editor.highlight");
  const QString activeLineTextColor =
      UiUtils::getThemeColor(themeManager, "editor.gutter.foreground.active");

  const RenderTheme theme = {textColor, activeLineTextColor, accentColor,
                             highlightColor};

  const auto measureWidth = [this](const QString &s) {
    return fontMetrics.horizontalAdvance(s);
  };
  const RenderState state = {
      QStringList(),  cursors,          selections, theme,       lineCount,
      verticalOffset, horizontalOffset, lineHeight, fontAscent,  fontDescent,
      font,           hasFocus,         isEmpty,    measureWidth};

  renderer->paint(painter, state, ctx);
}
