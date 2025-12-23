#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "features/main_window/workspace_ui_handles.h"
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>

class ThemeManager : public QObject {
  Q_OBJECT

public:
  explicit ThemeManager(neko::ThemeManager *nekoThemeManager,
                        const WorkspaceUiHandles *uiHandles,
                        QObject *parent = nullptr);
  ~ThemeManager();

  void applyTheme(const std::string &themeName);

signals:
  void themeChanged();

private:
  neko::ThemeManager *nekoThemeManager;
  const WorkspaceUiHandles *uiHandles;
};

#endif
