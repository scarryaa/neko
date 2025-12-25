#ifndef CONTEXT_MENU_WIDGET_H
#define CONTEXT_MENU_WIDGET_H

#include "features/context_menu/context_menu_frame.h"
#include "features/context_menu/context_menu_item.h"
#include "theme/theme_types.h"
#include "types/ffi_types_fwd.h"
#include "types/qt_types_fwd.h"
#include <QPoint>
#include <QString>
#include <QVector>
#include <QWidget>

class ThemeProvider;

QT_FWD(QVBoxLayout, QEvent, QKeyEvent)

class ContextMenuWidget : public QWidget {
  Q_OBJECT

public:
  struct ContextMenuProps {
    ThemeProvider *themeProvider;
    neko::ConfigManager *configManager;
  };

  explicit ContextMenuWidget(const ContextMenuProps &props,
                             QWidget *parent = nullptr);
  ~ContextMenuWidget() override;

  void setItems(const QVector<ContextMenuItem> &items);
  void showMenu(const QPoint &position);
  void setAndApplyTheme(const ContextMenuTheme &newTheme);

signals:
  void actionTriggered(const QString &actionId);

protected:
  bool eventFilter(QObject *watched, QEvent *event) override;
  void keyPressEvent(QKeyEvent *event) override;
  void showEvent(QShowEvent *event) override;

private:
  void connectSignals();
  void clearRows();

  ContextMenuTheme theme;

  ThemeProvider *themeProvider;
  ContextMenuFrame *mainFrame;
  neko::ConfigManager *configManager;
  QVBoxLayout *layout;
};

#endif
