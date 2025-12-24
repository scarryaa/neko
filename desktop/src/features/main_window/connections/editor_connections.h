#ifndef EDITOR_CONNECTIONS_H
#define EDITOR_CONNECTIONS_H

#include "features/editor/controllers/editor_controller.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/workspace_ui_handles.h"
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
