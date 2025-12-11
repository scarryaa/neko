#ifndef TITLE_BAR_WIDGET_H
#define TITLE_BAR_WIDGET_H

#include "neko_core.h"
#include "utils/gui_utils.h"
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
  explicit TitleBarWidget(NekoConfigManager *configManager,
                          NekoThemeManager *themeManager,
                          QWidget *parent = nullptr);
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

  NekoThemeManager *themeManager;
  NekoConfigManager *configManager;
  double m_height;
  QPushButton *m_directorySelectionButton;
  QPoint m_clickPos;
  QString m_currentDir;
};

#endif // TITLE_BAR_WIDGET_H
