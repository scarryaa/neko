#include "main_window.h"
#include "core/bridge/app_bridge.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/editor/bridge/editor_bridge.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/bridge/file_tree_bridge.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/connections/editor_connections.h"
#include "features/main_window/connections/file_explorer_connections.h"
#include "features/main_window/connections/main_window_connections.h"
#include "features/main_window/connections/theme_connections.h"
#include "features/main_window/connections/ui_style_connections.h"
#include "features/main_window/connections/workspace_connections.h"
#include "features/main_window/controllers/command_executor.h"
#include "features/main_window/controllers/command_manager.h"
#include "features/main_window/controllers/shortcuts_manager.h"
#include "features/main_window/controllers/ui_style_manager.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/main_window/layout/main_window_layout_builder.h"
#include "features/main_window/services/app_config_service.h"
#include "features/main_window/services/dialog_service.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/bridge/tab_bridge.h"
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
      themeManager(neko::new_theme_manager()),
      shortcutsManager(neko::new_shortcuts_manager()) {
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  commandRegistry = CommandRegistry();
  contextMenuRegistry = ContextMenuRegistry();

  auto *appBridge =
      new AppBridge({.configManager = *configManager, .rootPath = ""});

  rust::Box<neko::EditorController> editorController =
      appBridge->getEditorController();
  rust::Box<neko::TabController> tabController = appBridge->getTabController();
  editorBridge = new EditorBridge(EditorBridge::EditorBridgeProps{
      .editorController = std::move(editorController)});

  auto *tabBridge = new TabBridge(
      TabBridge::TabBridgeProps{.tabController = std::move(tabController)});

  appConfigService =
      new AppConfigService({.configManager = &*configManager}, this);
  auto *commandExecutor =
      new CommandExecutor({.configManager = &*configManager,
                           .themeManager = &*themeManager,
                           .appBridge = &*appBridge,
                           .appConfigService = appConfigService});
  themeProvider = new ThemeProvider({.themeManager = &*themeManager}, this);
  uiStyleManager = new UiStyleManager(
      {.appConfigService = appConfigService, .themeProvider = themeProvider},
      this);

  applyTheme();
  setupWidgets(tabBridge, appBridge);

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
          .tabBridge = tabBridge,
          .appBridge = appBridge,
          .editorBridge = editorBridge,
          .appConfigService = appConfigService,
          .commandExecutor = commandExecutor,
          .uiHandles = uiHandles,
      },
      this);

  auto *commandManager = new CommandManager(CommandManager::CommandManagerProps{
      .commandRegistry = &commandRegistry,
      .contextMenuRegistry = &contextMenuRegistry,
      .workspaceCoordinator = workspaceCoordinator,
      .appBridge = appBridge,
  });

  auto *qtShortcutsManager = new ShortcutsManager(
      {
          .actionOwner = this,
          .shortcutsManager = &*shortcutsManager,
          .workspaceCoordinator = workspaceCoordinator,
          .tabBridge = tabBridge,
          .uiHandles = &uiHandles,
      },
      this);

  connectSignals();

  workspaceCoordinator->applyInitialState();
  themeProvider->reload();
}

void MainWindow::setupWidgets(TabBridge *tabBridge, AppBridge *appBridge) {
  auto themes = themeProvider->getCurrentThemes();
  auto fonts = uiStyleManager->getCurrentFonts();
  auto snapshot = appConfigService->getSnapshot();
  const bool fileExplorerShown = snapshot.file_explorer.shown;

  auto *fileTreeBridge = new FileTreeBridge(
      {.fileTreeController = appBridge->getFileTreeController()}, this);
  const auto jumpHints = WorkspaceCoordinator::buildJumpHintRows();

  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(
      {.font = fonts.interfaceFont, .theme = themes.titleBarTheme}, this);
  fileExplorerWidget =
      new FileExplorerWidget({.fileTreeBridge = fileTreeBridge,
                              .font = fonts.fileExplorerFont,
                              .theme = themes.fileExplorerTheme},
                             this);
  commandPaletteWidget =
      new CommandPaletteWidget({.appBridge = appBridge,
                                .font = fonts.commandPaletteFont,
                                .theme = themes.commandPaletteTheme,
                                .jumpHints = jumpHints},
                               this);
  editorWidget = new EditorWidget({.editorBridge = editorBridge,
                                   .font = fonts.editorFont,
                                   .theme = themes.editorTheme},
                                  this);
  gutterWidget = new GutterWidget({.editorBridge = editorBridge,
                                   .theme = themes.gutterTheme,
                                   .font = fonts.editorFont},
                                  this);
  statusBarWidget =
      new StatusBarWidget({.editorBridge = editorBridge,
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
                                   .tabBridge = tabBridge},
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
                         .editorBridge = editorBridge,
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
