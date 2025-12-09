#ifndef TITLEBARWIDGET_H
#define TITLEBARWIDGET_H

#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QWidget>

class TitleBarWidget : public QWidget {
  Q_OBJECT

public:
  TitleBarWidget(QWidget *parent = nullptr);
  ~TitleBarWidget();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  QPoint m_clickPos;

  double TITLEBAR_HEIGHT = 32.0;
  QColor COLOR_BLACK = QColor(0, 0, 0);
  QColor BORDER_COLOR = QColor("#3c3c3c");
};

#endif //  TITLEBARWIDGET_H
