#ifndef WORKSPACE_CONNECTIONS_H
#define WORKSPACE_CONNECTIONS_H

class UiHandles;
class WorkspaceCoordinator;

#include <QObject>

class WorkspaceConnections : public QObject {
  Q_OBJECT

public:
  struct WorkspaceConnectionsProps {
    UiHandles &uiHandles;
    WorkspaceCoordinator *workspaceCoordinator;
  };

  explicit WorkspaceConnections(const WorkspaceConnectionsProps &props,
                                QObject *parent = nullptr);
  ~WorkspaceConnections() override = default;
};

#endif
