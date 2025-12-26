#ifndef UI_HANDLES_H
#define UI_HANDLES_H

class EditorWidget;
class GutterWidget;
class TabBarWidget;
class FileExplorerWidget;
class StatusBarWidget;
class CommandPaletteWidget;
class TitleBarWidget;

#include "types/qt_types_fwd.h"

QT_FWD(QSplitter, QPushButton, QWidget)

struct UiHandles {
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
