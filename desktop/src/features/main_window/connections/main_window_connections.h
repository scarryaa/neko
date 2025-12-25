#ifndef MAIN_WINDOW_CONNECTIONS_H
#define MAIN_WINDOW_CONNECTIONS_H

class WorkspaceCoordinator;
class WorkspaceUiHandles;
class ThemeProvider;

#include <QObject>

class MainWindowConnections : public QObject {
  Q_OBJECT

public:
  struct MainWindowConnectionsProps {
    const WorkspaceUiHandles &uiHandles;
    WorkspaceCoordinator *workspaceCoordinator;
    ThemeProvider *themeProvider;
  };

  explicit MainWindowConnections(const MainWindowConnectionsProps &props,
                                 QObject *parent = nullptr);

  ~MainWindowConnections() override = default;
};

#endif
