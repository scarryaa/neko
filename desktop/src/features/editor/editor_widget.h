#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QScrollArea>
#include <QScrollBar>
#include <neko_core.h>

class EditorWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit EditorWidget(NekoEditor *editor, QWidget *parent = nullptr);
  ~EditorWidget();

  void updateDimensionsAndRepaint();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

signals:
  void fontSizeChanged(qreal newSize);
  void lineCountChanged();

private:
  struct ViewportContext {
    double lineHeight;
    int firstVisibleLine;
    int lastVisibleLine;
    double verticalOffset;
    double horizontalOffset;
  };

  void drawText(QPainter *painter, const ViewportContext &ctx);
  void drawCursor(QPainter *painter, const ViewportContext &ctx);
  void drawSelections(QPainter *painter, const ViewportContext &ctx);
  void drawSelection(QPainter *painter, double x1, double x2, double y1,
                     double y2);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();

  void handleViewportUpdate();
  void scrollToCursor();
  double measureContent();

  NekoEditor *editor;
  QFont *font;
  QFontMetricsF fontMetrics;

  int EXTRA_VERTICAL_LINES = 1;
  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  QColor CURSOR_COLOR = QColor(66, 181, 212);
  QColor TEXT_COLOR = QColor(235, 235, 235);
  QColor SELECTION_COLOR = QColor(66, 181, 212, 50);
  double VIEWPORT_PADDING = 74.0;
};

#endif
