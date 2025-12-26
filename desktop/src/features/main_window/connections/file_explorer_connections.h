#ifndef FILE_EXPLORER_CONNECTIONS_H
#define FILE_EXPLORER_CONNECTIONS_H

class AppConfigService;
class WorkspaceUiHandles;

#include <QObject>

class FileExplorerConnections : public QObject {
  Q_OBJECT

public:
  struct FileExplorerConnectionsProps {
    const WorkspaceUiHandles &uiHandles;
    AppConfigService *appConfigService;
  };

  explicit FileExplorerConnections(const FileExplorerConnectionsProps &props,
                                   QObject *parent = nullptr);
  ~FileExplorerConnections() override = default;

signals:
  void savedDirectoryLoaded(const std::string &newSavedDir);
};

#endif
