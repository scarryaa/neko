#ifndef CORE_SERVICES_H
#define CORE_SERVICES_H

#include "types/ffi_types_fwd.h"

class ThemeProvider;
class CommandRegistry;
class ContextMenuRegistry;
class TabController;
class WorkspaceController;
class WorkspaceCoordinator;

struct CoreServices {
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  ThemeProvider *themeProvider;
  CommandRegistry *commandRegistry;
  ContextMenuRegistry *contextMenuRegistry;
  TabController *tabController;
  WorkspaceController *workspaceController;
  WorkspaceCoordinator *workspaceCoordinator;
};

#endif
