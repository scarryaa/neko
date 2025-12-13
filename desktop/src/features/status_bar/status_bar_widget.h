#ifndef STATUS_BAR_WIDGET_H
#define STATUS_BAR_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QHBoxLayout>
#include <QPainter>
#include <QWidget>

class StatusBarWidget : public QWidget {
  Q_OBJECT

public:
  explicit StatusBarWidget(neko::ConfigManager &configManager,
                           neko::ThemeManager &themeManager,
                           QWidget *parent = nullptr);
  ~StatusBarWidget();

protected:
  void paintEvent(QPaintEvent *event) override;

private:
  neko::ThemeManager &themeManager;
  neko::ConfigManager &configManager;
  double m_height;
};

#endif // STATUS_BAR_WIDGET_H
