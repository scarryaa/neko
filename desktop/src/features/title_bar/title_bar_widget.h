#ifndef TITLE_BAR_WIDGET_H
#define TITLE_BAR_WIDGET_H

#include "utils/gui_utils.h"
#include <QHBoxLayout>
#include <QMouseEvent>
#include <QPainter>
#include <QPen>
#include <QPushButton>
#include <QString>
#include <QWidget>
#include <neko-core/src/ffi/bridge.rs.h>
#include <string>

class TitleBarWidget : public QWidget {
  Q_OBJECT

public:
  explicit TitleBarWidget(neko::ConfigManager &configManager,
                          neko::ThemeManager &themeManager,
                          QWidget *parent = nullptr);
  ~TitleBarWidget();

  void applyTheme();

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

  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  double m_height;
  QPushButton *m_directorySelectionButton;
  QPoint m_clickPos;
  QString m_currentDir;
};

#endif // TITLE_BAR_WIDGET_H
