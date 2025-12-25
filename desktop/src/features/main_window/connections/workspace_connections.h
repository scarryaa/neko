#ifndef WORKSPACE_CONNECTIONS_H
#define WORKSPACE_CONNECTIONS_H

class WorkspaceUiHandles;
class WorkspaceCoordinator;

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
