#ifndef COMMAND_REGISTRY_H
#define COMMAND_REGISTRY_H

#include <QHash>

class CommandRegistry {
public:
  using Fn = std::function<void(const QVariant &)>;

  void registerCommand(QString id, Fn fn);
  void run(const QString &id, const QVariant &ctx) const;

private:
  QHash<QString, Fn> cmds;
};

#endif
