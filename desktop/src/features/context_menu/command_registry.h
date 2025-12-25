#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include <QHash>
#include <QString>
#include <QVariant>

class CommandRegistry {
public:
  using Fn = std::function<void(const QVariant &)>;

  void registerCommand(const QString &commandId, Fn commandFn);
  void run(const QString &commandId, const QVariant &ctx) const;

private:
  QHash<QString, Fn> cmds;
};

#endif
