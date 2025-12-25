#ifndef EDITOR_CONNECTIONS_H
#define EDITOR_CONNECTIONS_H

class EditorController;
class WorkspaceCoordinator;
class WorkspaceUiHandles;

#include <QObject>

class EditorConnections : public QObject {
  Q_OBJECT

public:
  struct EditorConnectionsProps {
    const WorkspaceUiHandles &uiHandles;
    EditorController *editorController;
    WorkspaceCoordinator *workspaceCoordinator;
  };

  explicit EditorConnections(const EditorConnectionsProps &props,
                             QObject *parent = nullptr);
  ~EditorConnections() override = default;
};

#endif
