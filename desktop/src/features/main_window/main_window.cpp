#include "main_window.h"
#include "features/context_menu/providers/tab_context.h"
#include "features/main_window/connections/editor_connections.h"
#include "features/main_window/connections/file_explorer_connections.h"
#include "features/main_window/connections/main_window_connections.h"
#include "features/main_window/connections/theme_connections.h"
#include "features/main_window/connections/workspace_connections.h"
#include "features/main_window/controllers/shortcuts_manager.h"
#include "utils/gui_utils.h"
#include "utils/mac_utils.h"

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
  qtThemeManager = new ThemeManager(&*themeManager, &uiHandles, this);

  MainWindow::registerCommands(commandManager);
  connectSignals();
  applyTheme();

  qtShortcutsManager->setUpKeyboardShortcuts();
  workspaceCoordinator->applyInitialState();
}

void MainWindow::setupWidgets(neko::Editor *editor,
                              FileTreeController *fileTreeController,
                              TabController *tabController) {
  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(*configManager, *themeManager, this);
  fileExplorerWidget = new FileExplorerWidget(
      fileTreeController, *configManager, *themeManager, this);
  commandPaletteWidget =
      new CommandPaletteWidget(*themeManager, *configManager, this);
  editorWidget =
      new EditorWidget(editorController, *configManager, *themeManager, this);
  gutterWidget =
      new GutterWidget(editorController, *configManager, *themeManager, this);
  statusBarWidget = new StatusBarWidget(editorController, *configManager,
                                        *themeManager, this);
  tabBarContainer = new QWidget(this);
  tabBarWidget =
      new TabBarWidget(*configManager, *themeManager, contextMenuRegistry,
                       commandRegistry, tabController, tabBarContainer);
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
  QString accentMutedColor =
      UiUtils::getThemeColor(*themeManager, "ui.accent.muted");
  QString foregroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.foreground");
  QString emptyStateBackgroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.background");

  QString emptyStateStylesheet =
      QString("QWidget { background-color: %1; }"
              "QPushButton { background-color: %2; border-radius: 4px; color: "
              "%3; }")
          .arg(emptyStateBackgroundColor, accentMutedColor, foregroundColor);
  emptyStateWidget->setStyleSheet(emptyStateStylesheet);
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
  qtThemeManager->applyTheme(std::string(currentTheme.c_str()));
  update();
}

void MainWindow::connectSignals() {
  new EditorConnections(uiHandles, editorController, workspaceCoordinator,
                        this);
  new MainWindowConnections(uiHandles, workspaceCoordinator, qtThemeManager,
                            this);
  new FileExplorerConnections(uiHandles, this);
  new WorkspaceConnections(uiHandles, workspaceCoordinator, this);
  new ThemeConnections(uiHandles, qtThemeManager, this);
}
