#ifndef FILE_EXPLORER_CONNECTIONS_H
#define FILE_EXPLORER_CONNECTIONS_H

class WorkspaceUiHandles;

#include <QObject>

class FileExplorerConnections : public QObject {
  Q_OBJECT

public:
  struct FileExplorerConnectionsProps {
    const WorkspaceUiHandles &uiHandles;
  };

  explicit FileExplorerConnections(const FileExplorerConnectionsProps &props,
                                   QObject *parent = nullptr);
  ~FileExplorerConnections() override = default;
};

#endif
