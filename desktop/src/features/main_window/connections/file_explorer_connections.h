#ifndef FILE_EXPLORER_CONNECTIONS_H
#define FILE_EXPLORER_CONNECTIONS_H

#include "features/main_window/workspace_ui_handles.h"
#include <QObject>

class FileExplorerConnections : public QObject {
  Q_OBJECT

public:
  explicit FileExplorerConnections(const WorkspaceUiHandles &uiHandles,
                                   QObject *parent = nullptr);
  ~FileExplorerConnections() override = default;
};

#endif
