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
          &GutterWidget::redraw);
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          &GutterWidget::redraw);
}

GutterWidget::~GutterWidget() {}

void GutterWidget::redraw() { viewport()->update(); }

void GutterWidget::onBufferChanged() { redraw(); }

void GutterWidget::onCursorChanged() { redraw(); }

void GutterWidget::onSelectionChanged() { redraw(); }

void GutterWidget::onViewportChanged() { updateDimensions(); }

void GutterWidget::setEditor(neko::Editor *newEditor) { editor = newEditor; }

QSize GutterWidget::sizeHint() const {
  return QSize(measureWidth() + VIEWPORT_PADDING, height());
}

double GutterWidget::measureWidth() const {
  if (!editor) {
    return 0;
  }

  int lineCount = editor->get_line_count();

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
  if (!editor) {
    return;
  }

  int lineCount = editor->get_line_count();
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
