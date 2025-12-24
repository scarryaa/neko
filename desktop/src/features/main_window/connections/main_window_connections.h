#ifndef MAIN_WINDOW_CONNECTIONS_H
#define MAIN_WINDOW_CONNECTIONS_H

#include "features/main_window/controllers/theme_manager.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/workspace_ui_handles.h"
#include <QObject>

class MainWindowConnections : public QObject {
  Q_OBJECT

public:
  explicit MainWindowConnections(const WorkspaceUiHandles &uiHandles,
                                 WorkspaceCoordinator *workspaceCoordinator,
                                 ThemeManager *qtThemeManager,
                                 QObject *parent = nullptr);

  ~MainWindowConnections() override = default;
};

#endif
