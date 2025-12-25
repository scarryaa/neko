#ifndef MAIN_WINDOW_CONNECTIONS_H
#define MAIN_WINDOW_CONNECTIONS_H

class WorkspaceCoordinator;
class WorkspaceUiHandles;
class ThemeProvider;

#include <QObject>

class MainWindowConnections : public QObject {
  Q_OBJECT

public:
  explicit MainWindowConnections(const WorkspaceUiHandles &uiHandles,
                                 WorkspaceCoordinator *workspaceCoordinator,
                                 ThemeProvider *themeProvider,
                                 QObject *parent = nullptr);

  ~MainWindowConnections() override = default;
};

#endif
