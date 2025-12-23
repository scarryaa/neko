#ifndef WORKSPACE_UI_HANDLES_H
#define WORKSPACE_UI_HANDLES_H

#include "features/command_palette/command_palette_widget.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/tab_bar_widget.h"

struct WorkspaceUiHandles {
  EditorWidget *editorWidget;
  GutterWidget *gutterWidget;
  TabBarWidget *tabBarWidget;
  QWidget *tabBarContainerWidget;
  QWidget *emptyStateWidget;
  FileExplorerWidget *fileExplorerWidget;
  StatusBarWidget *statusBarWidget;
  CommandPaletteWidget *commandPaletteWidget;
  QWidget *window;
};

#endif
