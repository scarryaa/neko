#include "main_window.h"
#include "controllers/app_state_controller.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/context_menu/command_registry.h"
#include "features/context_menu/context_menu_registry.h"
#include "features/context_menu/providers/tab_context.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/controllers/file_tree_controller.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/connections/editor_connections.h"
#include "features/main_window/connections/file_explorer_connections.h"
#include "features/main_window/connections/main_window_connections.h"
#include "features/main_window/connections/theme_connections.h"
#include "features/main_window/connections/workspace_connections.h"
#include "features/main_window/controllers/command_manager.h"
#include "features/main_window/controllers/shortcuts_manager.h"
#include "features/main_window/controllers/workspace_controller.h"
#include "features/main_window/controllers/workspace_coordinator.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include "features/title_bar/title_bar_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "theme/theme_provider.h"
#include "utils/gui_utils.h"
#include "utils/mac_utils.h"
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

Q_DECLARE_METATYPE(TabContext);

// TODO(scarlet): StatusBar signals/tab inform and MainWindow editor ref
// are messy and need to be cleaned up. Also consider "rearchitecting"
// to use the AppState consistently and extract common MainWindow/StatusBar
// functionality.
// TODO(scarlet): Auto detect config file save in editor
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), appState(neko::new_app_state("")),
      themeManager(neko::new_theme_manager()),
      configManager(neko::new_config_manager()),
      shortcutsManager(neko::new_shortcuts_manager()) {
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  commandRegistry = CommandRegistry();
  contextMenuRegistry = ContextMenuRegistry();
  auto *appStateController = new AppStateController(&*appState);
  auto *tabController = new TabController(&*appState);

  neko::Editor *editor = &appStateController->getActiveEditorMut();
  neko::FileTree *fileTree = &appStateController->getFileTreeMut();

  auto *fileTreeController = new FileTreeController(fileTree, this);
  editorController = new EditorController(editor);
  workspaceController = new WorkspaceController(
      tabController,
      WorkspaceUi{
          .promptSaveAsPath =
              [this](int tabId) {
                return QFileDialog::getSaveFileName(this, tr("Save As"),
                                                    QDir::homePath());
              },
          .confirmCloseTabs =
              [this](const QList<int> &ids) {
                return workspaceCoordinator->showTabCloseConfirmationDialog(
                    ids);
              }});
  themeProvider = new ThemeProvider(&*themeManager, this);

  applyTheme();
  setupWidgets(editor, fileTreeController, tabController);
  setupLayout();

  uiHandles = WorkspaceUiHandles{
      editorWidget,          gutterWidget,         tabBarWidget,
      tabBarContainer,       emptyStateWidget,     fileExplorerWidget,
      statusBarWidget,       commandPaletteWidget, this->window(),
      titleBarWidget,        mainSplitter,         newTabButton,
      emptyStateNewTabButton};

  workspaceCoordinator = new WorkspaceCoordinator(
      workspaceController, tabController, appStateController, editorController,
      &*configManager, &*themeManager, &uiHandles, this);
  auto *commandManager = new CommandManager(
      &commandRegistry, &contextMenuRegistry, workspaceCoordinator);
  auto *qtShortcutsManager =
      new ShortcutsManager(this, &*shortcutsManager, workspaceCoordinator,
                           tabController, &uiHandles, this);

  MainWindow::registerCommands(commandManager);
  connectSignals();

  qtShortcutsManager->setUpKeyboardShortcuts();
  workspaceCoordinator->applyInitialState();
  themeProvider->reload();
}

void MainWindow::setupWidgets(neko::Editor *editor,
                              FileTreeController *fileTreeController,
                              TabController *tabController) {
  const auto &titleBarTheme = themeProvider->getTitleBarTheme();
  const auto &statusBarTheme = themeProvider->getStatusBarTheme();
  const auto &fileExplorerTheme = themeProvider->getFileExplorerTheme();
  const auto &commandPaletteTheme = themeProvider->getCommandPaletteTheme();
  const auto &editorTheme = themeProvider->getEditorTheme();
  const auto &gutterTheme = themeProvider->getGutterTheme();
  const auto &tabBarTheme = themeProvider->getTabBarTheme();
  const auto &tabTheme = themeProvider->getTabTheme();

  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(*configManager, titleBarTheme, this);
  fileExplorerWidget = new FileExplorerWidget(
      fileTreeController, *configManager, fileExplorerTheme, this);
  commandPaletteWidget =
      new CommandPaletteWidget(commandPaletteTheme, *configManager, this);
  editorWidget =
      new EditorWidget(editorController, *configManager, editorTheme, this);
  gutterWidget =
      new GutterWidget(editorController, *configManager, gutterTheme, this);
  statusBarWidget = new StatusBarWidget(editorController, *configManager,
                                        statusBarTheme, this);
  tabBarContainer = new QWidget(this);
  tabBarWidget = new TabBarWidget(*configManager, tabBarTheme, tabTheme,
                                  contextMenuRegistry, commandRegistry,
                                  tabController, tabBarContainer);
}

void MainWindow::setupLayout() {
  auto *mainContainer = new QWidget(this);
  auto *mainLayout = new QVBoxLayout(mainContainer);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(titleBarWidget);

  auto *editorSideContainer = new QWidget(this);
  auto *editorSideLayout = new QVBoxLayout(editorSideContainer);
  editorSideLayout->setContentsMargins(0, 0, 0, 0);
  editorSideLayout->setSpacing(0);

  editorSideLayout->addWidget(buildTabBarSection());
  QWidget *emptyState = buildEmptyStateSection();
  editorSideLayout->addWidget(buildEditorSection(emptyState));

  auto *splitter = buildSplitter(editorSideContainer);

  mainLayout->addWidget(splitter);
  mainLayout->addWidget(statusBarWidget);
  setCentralWidget(mainContainer);
}

QWidget *MainWindow::buildTabBarSection() {
  auto *tabBarLayout = new QHBoxLayout(tabBarContainer);
  tabBarLayout->setContentsMargins(0, 0, 0, 0);
  tabBarLayout->setSpacing(0);

  newTabButton = new QPushButton("+", tabBarContainer);

  QFont uiFont = UiUtils::loadFont(*configManager, neko::FontType::Interface);
  QFontMetrics fontMetrics(uiFont);
  int dynamicHeight = static_cast<int>(
      fontMetrics.height() + TOP_TAB_BAR_PADDING + BOTTOM_TAB_BAR_PADDING);

  newTabButton->setFixedSize(dynamicHeight, dynamicHeight);
  tabBarLayout->addWidget(tabBarWidget);
  tabBarLayout->addWidget(newTabButton);

  return tabBarContainer;
}

QWidget *MainWindow::buildEmptyStateSection() {
  auto *emptyLayout = new QVBoxLayout(emptyStateWidget);
  emptyLayout->setAlignment(Qt::AlignCenter);

  emptyStateNewTabButton = new QPushButton("New Tab", emptyStateWidget);
  emptyStateNewTabButton->setFixedSize(EMPTY_STATE_NEW_TAB_BUTTON_WIDTH,
                                       EMPTY_STATE_NEW_TAB_BUTTON_HEIGHT);

  emptyLayout->addWidget(emptyStateNewTabButton);
  emptyStateWidget->hide();
  return emptyStateWidget;
}

QWidget *MainWindow::buildEditorSection(QWidget *emptyState) {
  auto *editorContainer = new QWidget(this);
  auto *editorLayout = new QHBoxLayout(editorContainer);
  editorLayout->setContentsMargins(0, 0, 0, 0);
  editorLayout->setSpacing(0);
  editorLayout->addWidget(gutterWidget, 0);
  editorLayout->addWidget(editorWidget, 1);
  editorLayout->addWidget(emptyState);
  return editorContainer;
}

QSplitter *MainWindow::buildSplitter(QWidget *editorSideContainer) {
  auto *splitter = new QSplitter(Qt::Horizontal, this);
  mainSplitter = splitter;
  auto snapshot = configManager->get_config_snapshot();
  auto fileExplorerRight = snapshot.file_explorer_right;
  int savedSidebarWidth = static_cast<int>(snapshot.file_explorer_width);

  if (fileExplorerRight) {
    splitter->addWidget(editorSideContainer);
    splitter->addWidget(fileExplorerWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes(QList<int>()
                       << SPLITTER_LARGE_WIDTH << savedSidebarWidth);
  } else {
    splitter->addWidget(fileExplorerWidget);
    splitter->addWidget(editorSideContainer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>()
                       << savedSidebarWidth << SPLITTER_LARGE_WIDTH);
  }

  splitter->setHandleWidth(1);

  QString borderColor = UiUtils::getThemeColor(*themeManager, "ui.border");
  QString splitterStylesheet = QString("QSplitter::handle {"
                                       "  background-color: %1;"
                                       "  margin: 0px;"
                                       "}")
                                   .arg(borderColor);
  splitter->setStyleSheet(splitterStylesheet);

  QObject::connect(splitter, &QSplitter::splitterMoved, this,
                   [splitter, this](int pos, int index) {
                     QList<int> sizes = splitter->sizes();

                     if (!sizes.isEmpty()) {
                       int newWidth = sizes.first();

                       auto snapshot = configManager->get_config_snapshot();
                       snapshot.file_explorer_width = newWidth;
                       configManager->apply_config_snapshot(snapshot);
                     }
                   });

  return splitter;
}

void MainWindow::registerCommands(CommandManager *commandManager) {
  qRegisterMetaType<TabContext>("TabContext");

  commandManager->registerProviders();
  commandManager->registerCommands();
}

void MainWindow::applyTheme() {
  auto snapshot = configManager->get_config_snapshot();
  auto currentTheme = snapshot.current_theme;
  themeManager->set_theme(currentTheme);

  themeProvider->reload();
  update();
}

void MainWindow::connectSignals() {
  new EditorConnections(uiHandles, editorController, workspaceCoordinator,
                        this);
  new MainWindowConnections(uiHandles, workspaceCoordinator, themeProvider,
                            this);
  new FileExplorerConnections(uiHandles, this);
  new WorkspaceConnections(uiHandles, workspaceCoordinator, this);
  new ThemeConnections(uiHandles, themeProvider, this);
}
