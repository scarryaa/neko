#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include "neko_core.h"
#include "utils/gui_utils.h"
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

class TabWidget : public QWidget {
  Q_OBJECT

public:
  explicit TabWidget(const QString &title, int index,
                     NekoConfigManager *configManager,
                     NekoThemeManager *themeManager, QWidget *parent = nullptr);
  void setActive(bool active);
  void setModified(bool modified);

signals:
  void clicked();
  void closeRequested();

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;

private:
  double measureText(QString text);

  NekoConfigManager *configManager;
  NekoThemeManager *themeManager;
  QString title;
  bool isModified = false;
  int index;
  bool isActive;
  bool isHovered = false;
  bool isCloseHovered = false;
};

#endif // TAB_WIDGET_H
