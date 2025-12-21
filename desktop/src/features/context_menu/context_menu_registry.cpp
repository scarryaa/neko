#include "features/context_menu/context_menu_registry.h"

void ContextMenuRegistry::registerProvider(const QString &key, ProviderFn fn) {
  providers[key] = std::move(fn);
}

QVector<ContextMenuItem> ContextMenuRegistry::build(const QString &key,
                                                    const QVariant &ctx) const {
  auto it = providers.find(key);
  if (it == providers.end())
    return {};
  return it.value()(ctx);
}
