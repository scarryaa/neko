#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

class CommandRegistry;
class ContextMenuRegistry;
class WorkspaceCoordinator;
class AppStateController;

#include <QApplication>

class CommandManager {
public:
  explicit CommandManager(CommandRegistry *commandRegistry,
                          ContextMenuRegistry *contextMenuRegistry,
                          WorkspaceCoordinator *workspaceCoordinator,
                          AppStateController *appStateController);
  ~CommandManager() = default;

  void registerCommands();
  void registerProviders();

private:
  CommandRegistry *commandRegistry;
  WorkspaceCoordinator *workspaceCoordinator;
  ContextMenuRegistry *contextMenuRegistry;
  AppStateController *appStateController;
};

#endif
