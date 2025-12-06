#ifndef EDITORWIDGET_H
#define EDITORWIDGET_H

#include <QScrollArea>
#include <QScrollBar>
#include <neko_core.h>

class EditorWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit EditorWidget(QWidget *parent = nullptr);
  ~EditorWidget();

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

private:
  void drawText(QPainter *painter);
  void drawCursor(QPainter *painter);
  void drawSelection(QPainter *painter);

  void increaseFontSize();
  void decreaseFontSize();
  void resetFontSize();

  void handleViewportUpdate();
  void scrollToCursor();
  double measureContent();

  NekoEditor *editor;
  QFont *font;
  QFontMetricsF fontMetrics;

  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  QColor CURSOR_COLOR = QColor(66, 181, 212);
  QColor SELECTION_COLOR = QColor(66, 181, 212, 50);
  double VIEWPORT_PADDING = 74.0;
};

#endif
