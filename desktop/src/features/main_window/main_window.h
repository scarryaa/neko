#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "features/command_palette/command_palette_widget.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/main_window/controllers/command_manager.h"
#include "features/main_window/controllers/shortcuts_manager.h"
#include "features/main_window/controllers/theme_manager.h"
#include "features/main_window/controllers/workspace_controller.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include "features/title_bar/title_bar_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "utils/mac_utils.h"
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private:
  void setupWidgets(neko::Editor *editor, neko::FileTree *fileTree);
  void setupLayout();
  QWidget *buildTabBarSection();
  QWidget *buildEmptyStateSection();
  QWidget *buildEditorSection(QWidget *emptyState);
  QSplitter *buildSplitter(QWidget *editorSideContainer);
  void connectSignals();

  rust::Box<neko::AppState> appState;
  rust::Box<neko::ThemeManager> themeManager;
  rust::Box<neko::ConfigManager> configManager;
  rust::Box<neko::ShortcutsManager> shortcutsManager;
  EditorController *editorController;
  TabController *tabController;
  WorkspaceCoordinator *workspaceCoordinator;
  WorkspaceController *workspaceController;
  AppStateController *appStateController;
  CommandRegistry commandRegistry;
  ContextMenuRegistry contextMenuRegistry;
  CommandManager *commandManager;
  ShortcutsManager *qtShortcutsManager;
  ThemeManager *qtThemeManager;

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
};

#endif // MAIN_WINDOW_H
