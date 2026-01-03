#ifndef COMMAND_MANAGER_H
#define COMMAND_MANAGER_H

#include "types/command_type.h"
class CommandRegistry;
class ContextMenuRegistry;
class WorkspaceCoordinator;
class AppBridge;

#include "types/qt_types_fwd.h"
#include <QApplication>

QT_FWD(QString, QVariant);

class CommandManager {
public:
  struct CommandManagerProps {
    CommandRegistry *commandRegistry;
    ContextMenuRegistry *contextMenuRegistry;
    WorkspaceCoordinator *workspaceCoordinator;
    AppBridge *appBridge;
  };

  explicit CommandManager(const CommandManagerProps &props);
  ~CommandManager() = default;

  void registerCommands();
  void registerProviders();

  void handleCommand(CommandType commandType, const QString &commandId,
                     const QVariant &variant);

private:
  CommandRegistry *commandRegistry;
  WorkspaceCoordinator *workspaceCoordinator;
  ContextMenuRegistry *contextMenuRegistry;
  AppBridge *appBridge;
};

#endif
