#ifndef FILE_EXPLORER_CONNECTIONS_H
#define FILE_EXPLORER_CONNECTIONS_H

class WorkspaceUiHandles;

#include "types/ffi_types_fwd.h"
#include <QObject>

class FileExplorerConnections : public QObject {
  Q_OBJECT

public:
  struct FileExplorerConnectionsProps {
    const WorkspaceUiHandles &uiHandles;
    neko::ConfigManager *configManager;
  };

  explicit FileExplorerConnections(const FileExplorerConnectionsProps &props,
                                   QObject *parent = nullptr);
  ~FileExplorerConnections() override = default;

signals:
  void savedDirectoryLoaded(const std::string &newSavedDir);
};

#endif
