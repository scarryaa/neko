#ifndef WORKSPACE_CONNECTIONS_H
#define WORKSPACE_CONNECTIONS_H

#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/workspace_ui_handles.h"
#include <QObject>

class WorkspaceConnections : public QObject {
  Q_OBJECT

public:
  explicit WorkspaceConnections(const WorkspaceUiHandles &uiHandles,
                                WorkspaceCoordinator *workspaceCoordinator,
                                QObject *parent = nullptr);
  ~WorkspaceConnections() override = default;
};

#endif
