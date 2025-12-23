#include "command_registry.h"

void CommandRegistry::registerCommand(const QString &commandId, Fn commandFn) {
  cmds[commandId] = std::move(commandFn);
}

void CommandRegistry::run(const QString &commandId, const QVariant &ctx) const {
  auto foundCommand = cmds.find(commandId);
  if (foundCommand != cmds.end()) {
    foundCommand.value()(ctx);
  }
}
