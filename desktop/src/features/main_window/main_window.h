#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

class EditorController;
class WorkspaceCoordinator;
class WorkspaceController;
class CommandManager;
class ThemeProvider;
class StatusBarWidget;
class TabBarWidget;
class TitleBarWidget;
class GutterWidget;
class EditorWidget;
class CommandPaletteWidget;
class FileExplorerWidget;
class TabController;
class AppController;
class AppConfigService;
class UiStyleManager;

#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "types/qt_types_fwd.h"
#include "ui_handles.h"
#include <QMainWindow>
#include <neko-core/src/ffi/bridge.rs.h>

QT_FWD(QPushButton, QSplitter, QWidget);

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow() override = default;

private:
  void setupWidgets(neko::Editor *editor, TabController *tabController,
                    AppController *appController);
  void applyTheme();
  void connectSignals();

  rust::Box<neko::ConfigManager> configManager;
  rust::Box<neko::AppState> appState;
  rust::Box<neko::ThemeManager> themeManager;
  rust::Box<neko::ShortcutsManager> shortcutsManager;
  EditorController *editorController;
  WorkspaceCoordinator *workspaceCoordinator;
  WorkspaceController *workspaceController;
  CommandRegistry commandRegistry;
  ContextMenuRegistry contextMenuRegistry;
  ThemeProvider *themeProvider;
  AppConfigService *appConfigService;
  UiStyleManager *uiStyleManager;

  QWidget *emptyStateWidget;
  QPushButton *emptyStateNewTabButton;
  FileExplorerWidget *fileExplorerWidget;
  CommandPaletteWidget *commandPaletteWidget;
  EditorWidget *editorWidget;
  GutterWidget *gutterWidget;
  TitleBarWidget *titleBarWidget;
  QWidget *tabBarContainer;
  TabBarWidget *tabBarWidget;
  StatusBarWidget *statusBarWidget;
  QPushButton *newTabButton;
  QSplitter *mainSplitter;
  UiHandles uiHandles;
};

#endif // MAIN_WINDOW_H
