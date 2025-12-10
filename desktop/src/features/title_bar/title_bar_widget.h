#ifndef TITLE_BAR_WIDGET_H
#define TITLE_BAR_WIDGET_H

#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <string>

class TitleBarWidget : public QWidget {
  Q_OBJECT

public:
  explicit TitleBarWidget(QWidget *parent = nullptr);
  ~TitleBarWidget();

public slots:
  void onDirChanged(std::string newDir);

signals:
  void directorySelectionButtonPressed();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;

private:
  void onDirectorySelectionButtonPressed();

  QPushButton *m_directorySelectionButton;
  QPoint m_clickPos;
  QString m_currentDir;

  double TITLEBAR_HEIGHT = 32.0;
  QColor COLOR_BLACK = QColor(0, 0, 0);
  QColor BORDER_COLOR = QColor("#3c3c3c");
};

#endif // TITLE_BAR_WIDGET_H
