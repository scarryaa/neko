#include "gutter_widget.h"

GutterWidget::GutterWidget(NekoEditor *editor, QWidget *parent)
    : QScrollArea(parent), editor(editor),
      font(new QFont("IBM Plex Mono", 15.0)), fontMetrics(*font) {
  setFocusPolicy(Qt::NoFocus);
  setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  setCornerWidget(nullptr);

  setStyleSheet("GutterWidget {"
                "  background: black;"
                "}");

  connect(verticalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
  connect(horizontalScrollBar(), &QScrollBar::valueChanged, this,
          [this]() { viewport()->repaint(); });
}

GutterWidget::~GutterWidget() {}

QSize GutterWidget::sizeHint() const {
  return QSize(measureContent() + VIEWPORT_PADDING, height());
}

double GutterWidget::measureContent() const {
  size_t lineCount = 0;
  neko_editor_get_line_count(editor, &lineCount);

  return fontMetrics.horizontalAdvance(QString::number(lineCount));
}

void GutterWidget::handleViewportUpdate() {
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

void GutterWidget::paintEvent(QPaintEvent *event) {
  QPainter painter(viewport());
  drawText(&painter);
}

void GutterWidget::drawText(QPainter *painter) {
  painter->setBrush(Qt::gray);
  painter->setFont(*font);

  size_t line_count;
  neko_editor_get_line_count(editor, &line_count);

  auto verticalOffset = verticalScrollBar()->value();
  auto horizontalOffset = horizontalScrollBar()->value();

  int maxLineNumber = line_count;
  int maxLineWidth =
      fontMetrics.horizontalAdvance(QString::number(maxLineNumber));
  double numWidth = fontMetrics.horizontalAdvance(QString::number(1));

  for (int i = 0; i < line_count; i++) {
    auto actualY =
        (i * fontMetrics.height()) +
        (fontMetrics.height() + fontMetrics.ascent() - fontMetrics.descent()) /
            2.0 -
        verticalOffset;

    QString lineNum = QString::number(i + 1);
    int lineNumWidth = fontMetrics.horizontalAdvance(lineNum);

    double x = (width() - maxLineWidth - numWidth) / 2.0 +
               (maxLineWidth - lineNumWidth) - horizontalOffset;

    painter->drawText(QPointF(x, actualY), lineNum);
  }
}
