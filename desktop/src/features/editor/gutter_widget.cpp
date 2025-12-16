#include "gutter_widget.h"
#include "utils/gui_utils.h"

GutterWidget::GutterWidget(neko::Editor *editor,
                           neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager, QWidget *parent)
    : QScrollArea(parent), editor(editor), configManager(configManager),
      renderer(new GutterRenderer()), themeManager(themeManager),
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

  rust::Vec<neko::CursorPosition> cursors = editor->get_cursor_positions();
  neko::Selection selections = editor->get_selection();

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
  RenderState state = {rust::Vec<rust::String>(),
                       cursors,
                       selections,
                       theme,
                       lineCount,
                       verticalOffset,
                       horizontalOffset,
                       lineHeight,
                       fontAscent,
                       fontDescent,
                       font,
                       hasFocus,
                       measureWidth};

  renderer->paint(painter, state, ctx);
}
