#ifndef CONTEXT_MENU_TYPES_H
#define CONTEXT_MENU_TYPES_H

#include <QString>
#include <cstdint>

enum class ContextMenuItemKind : uint8_t { Action, Separator };

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
