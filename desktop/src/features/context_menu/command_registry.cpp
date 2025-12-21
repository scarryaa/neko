#include "command_registry.h"

void CommandRegistry::registerCommand(QString id, Fn fn) {
  cmds[id] = std::move(fn);
}

void CommandRegistry::run(const QString &id, const QVariant &ctx) const {
  auto it = cmds.find(id);
  if (it != cmds.end())
    it.value()(ctx);
}
