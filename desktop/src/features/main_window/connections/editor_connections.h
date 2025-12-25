#ifndef EDITOR_CONNECTIONS_H
#define EDITOR_CONNECTIONS_H

class EditorController;
class WorkspaceCoordinator;
class WorkspaceUiHandles;

#include <QObject>

class EditorConnections : public QObject {
  Q_OBJECT

public:
  explicit EditorConnections(const WorkspaceUiHandles &uiHandles,
                             EditorController *editorController,
                             WorkspaceCoordinator *workspaceCoordinator,
                             QObject *parent = nullptr);
  ~EditorConnections() override = default;
};

#endif
