#include "main_window.h"

Q_DECLARE_METATYPE(TabContext);

// TODO(scarlet): StatusBar signals/tab inform and MainWindow editor ref
// are messy and need to be cleaned up. Also consider "rearchitecting"
// to use the AppState consistently and extract common MainWindow/StatusBar
// functionality.
// TODO(scarlet): Auto detect config file save in editor
// TODO(scarlet): Prevent config reset on updating model
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), appState(neko::new_app_state("")),
      themeManager(neko::new_theme_manager()),
      configManager(neko::new_config_manager()),
      shortcutsManager(neko::new_shortcuts_manager()) {
  qRegisterMetaType<TabContext>("TabContext");

  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  commandRegistry = CommandRegistry();
  contextMenuRegistry = ContextMenuRegistry();
  appStateController = new AppStateController(&*appState);
  tabController = new TabController(&*appState);

  neko::Editor *editor = &appStateController->getActiveEditorMut();
  neko::FileTree *fileTree = &appStateController->getFileTreeMut();

  fileTreeController = new FileTreeController(fileTree, this);
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

  setupWidgets(editor, fileTreeController);
  setupLayout();

  uiHandles = WorkspaceUiHandles{
      editorWidget,    gutterWidget,         tabBarWidget,
      tabBarContainer, emptyStateWidget,     fileExplorerWidget,
      statusBarWidget, commandPaletteWidget, this->window(),
      titleBarWidget,  mainSplitter,         newTabButton};

  workspaceCoordinator = new WorkspaceCoordinator(
      workspaceController, tabController, appStateController, editorController,
      &*configManager, &*themeManager, &uiHandles, this);
  commandManager = new CommandManager(&commandRegistry, &contextMenuRegistry,
                                      workspaceCoordinator);
  qtShortcutsManager =
      new ShortcutsManager(this, &*shortcutsManager, workspaceCoordinator,
                           tabController, &uiHandles, this);
  qtThemeManager = new ThemeManager(&*themeManager, &uiHandles, this);

  commandManager->registerProviders();
  commandManager->registerCommands();
  connectSignals();

  auto snapshot = configManager->get_config_snapshot();
  auto currentTheme = snapshot.current_theme;
  qtThemeManager->applyTheme(std::string(currentTheme.c_str()));
  update();

  qtShortcutsManager->setUpKeyboardShortcuts();
  workspaceCoordinator->applyInitialState();
}

void MainWindow::setupWidgets(neko::Editor *editor,
                              FileTreeController *fileTreeController) {
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

void MainWindow::connectSignals() {
  // NewTabButton -> WorkspaceCoordinator
  connect(newTabButton, &QPushButton::clicked, workspaceCoordinator,
          &WorkspaceCoordinator::newTab);

  // EmptyStateNewTabButton -> WorkspaceCoordinator
  connect(emptyStateNewTabButton, &QPushButton::clicked, workspaceCoordinator,
          &WorkspaceCoordinator::newTab);

  // WorkspaceCoordinator -> MainWindow
  connect(workspaceCoordinator, &WorkspaceCoordinator::themeChanged,
          qtThemeManager, &ThemeManager::applyTheme);

  // GutterWidget <-> EditorWidget
  connect(gutterWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          editorWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(editorWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          gutterWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(editorWidget, &EditorWidget::fontSizeChanged, gutterWidget,
          &GutterWidget::onEditorFontSizeChanged);

  // FileExplorerWidget -> MainWindow
  connect(fileExplorerWidget, &FileExplorerWidget::fileSelected,
          workspaceCoordinator, &WorkspaceCoordinator::fileSelected);

  // FileExplorerWidget <-> TitleBarWidget
  connect(fileExplorerWidget, &FileExplorerWidget::directorySelected,
          titleBarWidget, &TitleBarWidget::onDirChanged);

  // TitleBarWidget -> FileExplorerWidget
  connect(titleBarWidget, &TitleBarWidget::directorySelectionButtonPressed,
          fileExplorerWidget, &FileExplorerWidget::directorySelectionRequested);

  // EditorController -> MainWindow
  connect(editorController, &EditorController::bufferChanged,
          workspaceCoordinator, &WorkspaceCoordinator::bufferChanged);

  // StatusBarWidget -> MainWindow
  connect(statusBarWidget, &StatusBarWidget::fileExplorerToggled,
          workspaceCoordinator, &WorkspaceCoordinator::fileExplorerToggled);
  connect(workspaceCoordinator,
          &WorkspaceCoordinator::onFileExplorerToggledViaShortcut,
          statusBarWidget, &StatusBarWidget::onFileExplorerToggledExternally);
  connect(statusBarWidget, &StatusBarWidget::cursorPositionClicked,
          workspaceCoordinator, &WorkspaceCoordinator::cursorPositionClicked);

  // CommandPalette -> MainWindow
  connect(commandPaletteWidget, &CommandPaletteWidget::goToPositionRequested,
          workspaceCoordinator,
          &WorkspaceCoordinator::commandPaletteGoToPosition);
  connect(commandPaletteWidget, &CommandPaletteWidget::commandRequested,
          workspaceCoordinator, &WorkspaceCoordinator::commandPaletteCommand);

  // EditorController -> StatusBarWidget
  connect(editorController, &EditorController::cursorChanged, statusBarWidget,
          &StatusBarWidget::onCursorPositionChanged);

  // TODO(scarlet): Rework the tab update system to not rely on mass setting
  // all the tabs and have the TabBarWidget be in charge of mgmt/updates,
  // with each TabWidget in control of its repaints?
}
