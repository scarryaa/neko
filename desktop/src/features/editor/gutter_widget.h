#ifndef GUTTERWIDGET_H
#define GUTTERWIDGET_H

#include <QPainter>
#include <QScrollArea>
#include <QScrollBar>
#include <QString>
#include <QWheelEvent>
#include <neko_core.h>

class GutterWidget : public QScrollArea {
  Q_OBJECT

public:
  explicit GutterWidget(NekoEditor *editor, QWidget *parent = nullptr);
  ~GutterWidget();

  void updateDimensionsAndRepaint();

protected:
  QSize sizeHint() const override;
  void paintEvent(QPaintEvent *event) override;
  void wheelEvent(QWheelEvent *event) override;

public slots:
  void onEditorFontSizeChanged(qreal newSize);

private:
  double measureContent() const;
  void handleViewportUpdate();
  void drawText(QPainter *painter);

  void decreaseFontSize();
  void increaseFontSize();
  void resetFontSize();

  NekoEditor *editor;
  QFont *font;
  QFontMetricsF fontMetrics;

  double FONT_STEP = 2.0;
  double DEFAULT_FONT_SIZE = 15.0;
  double FONT_UPPER_LIMIT = 96.0;
  double FONT_LOWER_LIMIT = 6.0;
  double VIEWPORT_PADDING = 74.0;
  QColor TEXT_COLOR = QColor(80, 80, 80);
};

#endif // GUTTERWIDGET_H
