#include "features/context_menu/context_menu_registry.h"

void ContextMenuRegistry::registerProvider(const QString &key,
                                           ProviderFn providerFn) {
  providers[key] = std::move(providerFn);
}

QVector<ContextMenuItem> ContextMenuRegistry::build(const QString &key,
                                                    const QVariant &ctx) const {
  auto foundProvider = providers.find(key);
  if (foundProvider == providers.end()) {
    return {};
  }

  return foundProvider.value()(ctx);
}
