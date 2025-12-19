#ifndef TAB_WIDGET_H
#define TAB_WIDGET_H

#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include <QPaintEvent>
#include <QPainter>
#include <QWidget>

class TabWidget : public QWidget {
  Q_OBJECT

public:
  explicit TabWidget(const QString &title, int index,
                     neko::ConfigManager &configManager,
                     neko::ThemeManager &themeManager,
                     QWidget *parent = nullptr);
  void setActive(bool active);
  void setModified(bool modified);

signals:
  void clicked();
  void closeRequested(bool bypassConfirmation);

protected:
  void paintEvent(QPaintEvent *event) override;
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;

private:
  double measureText(QString text);

  neko::ConfigManager &configManager;
  neko::ThemeManager &themeManager;
  QString title;
  bool isModified = false;
  int index;
  bool isActive;
  bool isHovered = false;
  bool isCloseHovered = false;
};

#endif // TAB_WIDGET_H
