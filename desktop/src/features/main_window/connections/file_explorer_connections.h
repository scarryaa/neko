#ifndef FILE_EXPLORER_CONNECTIONS_H
#define FILE_EXPLORER_CONNECTIONS_H

// TODO(scarlet): Centralize these stubs
class AppConfigService;
class UiHandles;

#include <QObject>

class FileExplorerConnections : public QObject {
  Q_OBJECT

public:
  struct FileExplorerConnectionsProps {
    const UiHandles &uiHandles;
    AppConfigService *appConfigService;
  };

  explicit FileExplorerConnections(const FileExplorerConnectionsProps &props,
                                   QObject *parent = nullptr);
  ~FileExplorerConnections() override = default;

signals:
  void savedDirectoryLoaded(const QString &newSavedDir);
};

#endif
