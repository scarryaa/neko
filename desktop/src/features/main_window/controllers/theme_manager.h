#ifndef THEME_MANAGER_H
#define THEME_MANAGER_H

#include "features/main_window/workspace_ui_handles.h"
#include <QObject>
#include <neko-core/src/ffi/bridge.rs.h>

class ThemeManager : public QObject {
  Q_OBJECT

public:
  explicit ThemeManager(neko::ThemeManager *nekoThemeManager,
                        WorkspaceUiHandles uiHandles);
  ~ThemeManager();

  void applyTheme(const std::string &themeName);

private:
  neko::ThemeManager *nekoThemeManager;
  WorkspaceUiHandles uiHandles;
};

#endif
