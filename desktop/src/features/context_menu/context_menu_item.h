#ifndef CONTEXT_MENU_ITEM_H
#define CONTEXT_MENU_ITEM_H

#include "context_menu_item_kind.h"
#include <QString>

struct ContextMenuItem {
  ContextMenuItemKind kind = ContextMenuItemKind::Action;
  QString id;
  QString label;
  QString shortcut;
  QString iconKey;
  bool enabled = true;
  bool visible = true;
  bool checked = false;
};

#endif
