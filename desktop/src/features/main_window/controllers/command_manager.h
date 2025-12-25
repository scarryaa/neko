#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

class CommandRegistry;
class ContextMenuRegistry;
class WorkspaceCoordinator;
class AppStateController;

#include "types/qt_types_fwd.h"
#include <QApplication>

QT_FWD(QString, QVariant);

class CommandManager {
public:
  explicit CommandManager(CommandRegistry *commandRegistry,
                          ContextMenuRegistry *contextMenuRegistry,
                          WorkspaceCoordinator *workspaceCoordinator,
                          AppStateController *appStateController);
  ~CommandManager() = default;

  void registerCommands();
  void registerProviders();

  void handleTabCommand(const QString &commandId, const QVariant &variant);

private:
  CommandRegistry *commandRegistry;
  WorkspaceCoordinator *workspaceCoordinator;
  ContextMenuRegistry *contextMenuRegistry;
  AppStateController *appStateController;
};

#endif
