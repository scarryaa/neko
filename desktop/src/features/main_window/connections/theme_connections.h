#ifndef THEME_CONNECTIONS_H
#define THEME_CONNECTIONS_H

#include "features/main_window/controllers/theme_manager.h"
#include "features/main_window/workspace_ui_handles.h"
#include "theme/theme_provider.h"
#include <QObject>

class ThemeConnections : public QObject {
  Q_OBJECT

public:
  explicit ThemeConnections(const WorkspaceUiHandles &uiHandles,
                            ThemeManager *themeManager,
                            ThemeProvider *themeProvider,
                            QObject *parent = nullptr);
  ~ThemeConnections() override = default;
};

#endif
