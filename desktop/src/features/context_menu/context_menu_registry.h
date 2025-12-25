#ifndef CONTEXT_MENU_REGISTRY_H
#define CONTEXT_MENU_REGISTRY_H

#include "features/context_menu/context_menu_item.h"
#include <QHash>
#include <QString>
#include <QVariant>
#include <QVector>
#include <functional>

using ProviderFn = std::function<QVector<ContextMenuItem>(const QVariant &ctx)>;

class ContextMenuRegistry {
public:
  void registerProvider(const QString &key, ProviderFn providerFn);
  [[nodiscard]] QVector<ContextMenuItem> build(const QString &key,
                                               const QVariant &ctx) const;

private:
  QHash<QString, ProviderFn> providers;
};

#endif // CONTEXT_MENU_REGISTRY_H
