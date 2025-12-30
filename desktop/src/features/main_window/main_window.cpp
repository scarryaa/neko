#include "main_window.h"
#include "controllers/app_state_controller.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/controllers/file_tree_controller.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/connections/editor_connections.h"
#include "features/main_window/connections/file_explorer_connections.h"
#include "features/main_window/connections/main_window_connections.h"
#include "features/main_window/connections/theme_connections.h"
#include "features/main_window/connections/ui_style_connections.h"
#include "features/main_window/connections/workspace_connections.h"
#include "features/main_window/controllers/app_config_service.h"
#include "features/main_window/controllers/command_executor.h"
#include "features/main_window/controllers/command_manager.h"
#include "features/main_window/controllers/shortcuts_manager.h"
#include "features/main_window/controllers/ui_style_manager.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/layout/main_window_layout_builder.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include "features/title_bar/title_bar_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "theme/theme_provider.h"
#include "utils/mac_utils.h"
#include "utils/ui_utils.h"
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QHBoxLayout>
#include <QList>
#include <QPushButton>
#include <QSplitter>
#include <QString>
#include <QVBoxLayout>
#include <QWidget>

// TODO(scarlet): StatusBar signals/tab inform and MainWindow editor ref
// are messy and need to be cleaned up. Also consider "rearchitecting"
// to use the AppState consistently and extract common MainWindow/StatusBar
// functionality.
// TODO(scarlet): Auto detect config file save in editor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), configManager(neko::new_config_manager()),
      appState(neko::new_app_state("", *configManager)),
      themeManager(neko::new_theme_manager()),
      shortcutsManager(neko::new_shortcuts_manager()) {
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  commandRegistry = CommandRegistry();
  contextMenuRegistry = ContextMenuRegistry();

  auto *appStateController = new AppStateController({.appState = &*appState});

  neko::Editor *editor = &appStateController->getActiveEditorMut();
  editorController = new EditorController({.editor = editor});

  auto *tabController = new TabController({.appState = &*appState});

  appConfigService =
      new AppConfigService({.configManager = &*configManager}, this);
  auto *commandExecutor =
      new CommandExecutor({.configManager = &*configManager,
                           .themeManager = &*themeManager,
                           .appState = &*appState,
                           .appConfigService = appConfigService});
  themeProvider = new ThemeProvider({.themeManager = &*themeManager}, this);
  uiStyleManager = new UiStyleManager(
      {.appConfigService = appConfigService, .themeProvider = themeProvider},
      this);

  applyTheme();
  setupWidgets(editor, tabController, appStateController);

  // Layout
  MainWindowLayoutBuilder layoutBuilder(
      {.themeProvider = themeProvider,
       .uiStyleManager = uiStyleManager,
       .appConfigService = &*appConfigService},
      this);

  auto layoutResult =
      layoutBuilder.build({.titleBarWidget = titleBarWidget,
                           .tabBarWidget = tabBarWidget,
                           .editorWidget = editorWidget,
                           .gutterWidget = gutterWidget,
                           .fileExplorerWidget = fileExplorerWidget,
                           .statusBarWidget = statusBarWidget});

  setCentralWidget(layoutResult.centralWidget);

  tabBarContainer = layoutResult.tabBarContainer;
  newTabButton = layoutResult.newTabButton;
  emptyStateWidget = layoutResult.emptyStateWidget;
  emptyStateNewTabButton = layoutResult.emptyStateNewTabButton;
  mainSplitter = layoutResult.mainSplitter;

  uiHandles =
      UiHandles{editorWidget,          gutterWidget,         tabBarWidget,
                tabBarContainer,       emptyStateWidget,     fileExplorerWidget,
                statusBarWidget,       commandPaletteWidget, this->window(),
                titleBarWidget,        mainSplitter,         newTabButton,
                emptyStateNewTabButton};

  auto *dialogService = new DialogService(this);

  workspaceCoordinator = new WorkspaceCoordinator(
      {
          .tabController = tabController,
          .appStateController = appStateController,
          .editorController = editorController,
          .appConfigService = appConfigService,
          .commandExecutor = commandExecutor,
          .uiHandles = uiHandles,
          .dialogService = dialogService,
      },
      this);

  auto *commandManager = new CommandManager(CommandManager::CommandManagerProps{
      .commandRegistry = &commandRegistry,
      .contextMenuRegistry = &contextMenuRegistry,
      .workspaceCoordinator = workspaceCoordinator,
      .appStateController = appStateController,
  });

  auto *qtShortcutsManager = new ShortcutsManager(
      {
          .actionOwner = this,
          .shortcutsManager = &*shortcutsManager,
          .workspaceCoordinator = workspaceCoordinator,
          .tabController = tabController,
          .uiHandles = &uiHandles,
      },
      this);

  connectSignals();

  workspaceCoordinator->applyInitialState();
  themeProvider->reload();
}

void MainWindow::setupWidgets(neko::Editor *editor,
                              TabController *tabController,
                              AppStateController *appStateController) {
  auto themes = themeProvider->getCurrentThemes();
  auto fonts = uiStyleManager->getCurrentFonts();
  auto snapshot = appConfigService->getSnapshot();
  const bool fileExplorerShown = snapshot.file_explorer.shown;

  neko::FileTree *fileTree = &appStateController->getFileTreeMut();
  auto *fileTreeController =
      new FileTreeController({.fileTree = fileTree}, this);
  const auto jumpHints = WorkspaceCoordinator::buildJumpHintRows();

  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(
      {.font = fonts.interfaceFont, .theme = themes.titleBarTheme}, this);
  fileExplorerWidget =
      new FileExplorerWidget({.fileTreeController = fileTreeController,
                              .font = fonts.fileExplorerFont,
                              .theme = themes.fileExplorerTheme},
                             this);
  commandPaletteWidget =
      new CommandPaletteWidget({.font = fonts.commandPaletteFont,
                                .theme = themes.commandPaletteTheme,
                                .jumpHints = jumpHints},
                               this);
  editorWidget = new EditorWidget({.editorController = editorController,
                                   .font = fonts.editorFont,
                                   .theme = themes.editorTheme},
                                  this);
  gutterWidget = new GutterWidget({.editorController = editorController,
                                   .theme = themes.gutterTheme,
                                   .font = fonts.editorFont},
                                  this);
  statusBarWidget =
      new StatusBarWidget({.editorController = editorController,
                           .theme = themes.statusBarTheme,
                           .fileExplorerInitiallyShown = fileExplorerShown},
                          this);
  tabBarContainer = new QWidget(this);
  tabBarWidget = new TabBarWidget({.theme = themes.tabBarTheme,
                                   .tabTheme = themes.tabTheme,
                                   .font = fonts.interfaceFont,
                                   .themeProvider = themeProvider,
                                   .contextMenuRegistry = &contextMenuRegistry,
                                   .commandRegistry = &commandRegistry,
                                   .tabController = tabController},
                                  tabBarContainer);
}

void MainWindow::applyTheme() {
  auto snapshot = configManager->get_config_snapshot();
  auto currentTheme = snapshot.current_theme;
  themeManager->set_theme(currentTheme);

  themeProvider->reload();
  update();
}

void MainWindow::connectSignals() {
  new EditorConnections({.uiHandles = uiHandles,
                         .editorController = editorController,
                         .workspaceCoordinator = workspaceCoordinator},
                        this);
  new MainWindowConnections({.uiHandles = uiHandles,
                             .workspaceCoordinator = workspaceCoordinator,
                             .themeProvider = themeProvider},
                            this);
  new FileExplorerConnections(
      {.uiHandles = uiHandles, .appConfigService = appConfigService}, this);
  new WorkspaceConnections(
      {.uiHandles = uiHandles, .workspaceCoordinator = workspaceCoordinator},
      this);
  new ThemeConnections({.uiHandles = uiHandles, .themeProvider = themeProvider},
                       this);
  new UiStyleConnections({.uiHandles = uiHandles,
                          .uiStyleManager = uiStyleManager,
                          .appConfigService = appConfigService});
}
