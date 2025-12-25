#ifndef WORKSPACE_UI_HANDLES_H
#define WORKSPACE_UI_HANDLES_H

class EditorWidget;
class GutterWidget;
class TabBarWidget;
class FileExplorerWidget;
class StatusBarWidget;
class CommandPaletteWidget;
class TitleBarWidget;

#include "types/qt_types_fwd.h"

QT_FWD(QSplitter, QPushButton, QWidget)

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
  TitleBarWidget *titleBarWidget;
  QSplitter *mainSplitter;
  QPushButton *newTabButton;
  QPushButton *emptyStateNewTabButton;
};

#endif
