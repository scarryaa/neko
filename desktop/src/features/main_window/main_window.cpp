#include "main_window.h"

Q_DECLARE_METATYPE(TabContext);

// TODO: StatusBar signals/tab inform and MainWindow editor ref
// are messy and need to be cleaned up. Also consider "rearchitecting"
// to use the AppState consistently and extract common MainWindow/StatusBar
// functionality.
// TODO: Auto detect config file save in editor
// TODO: Prevent config reset on updating model
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

  setupWidgets(editor, fileTree);
  setupLayout();

  WorkspaceUiHandles uiHandles{
      editorWidget,    gutterWidget,         tabBarWidget,
      tabBarContainer, emptyStateWidget,     fileExplorerWidget,
      statusBarWidget, commandPaletteWidget, this->window()};

  workspaceCoordinator = new WorkspaceCoordinator(
      workspaceController, tabController, appStateController, editorController,
      &*configManager, &*themeManager, uiHandles, this);
  commandManager = new CommandManager(&commandRegistry, &contextMenuRegistry,
                                      workspaceCoordinator);

  commandManager->registerProviders();
  commandManager->registerCommands();
  connectSignals();

  auto snapshot = configManager->get_config_snapshot();
  auto currentTheme = snapshot.current_theme;
  applyTheme(std::string(currentTheme.c_str()));
  setupKeyboardShortcuts();
  workspaceCoordinator->applyInitialState();
}

MainWindow::~MainWindow() {}

void MainWindow::setupWidgets(neko::Editor *editor, neko::FileTree *fileTree) {
  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(*configManager, *themeManager, this);
  fileExplorerWidget =
      new FileExplorerWidget(fileTree, *configManager, *themeManager, this);
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
  QWidget *mainContainer = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(titleBarWidget);

  QWidget *editorSideContainer = new QWidget(this);
  QVBoxLayout *editorSideLayout = new QVBoxLayout(editorSideContainer);
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
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;

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
  emptyStateNewTabButton->setFixedSize(80, 35);

  emptyLayout->addWidget(emptyStateNewTabButton);
  emptyStateWidget->hide();
  return emptyStateWidget;
}

QWidget *MainWindow::buildEditorSection(QWidget *emptyState) {
  QWidget *editorContainer = new QWidget(this);
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
  int savedSidebarWidth = snapshot.file_explorer_width;

  if (fileExplorerRight) {
    splitter->addWidget(editorSideContainer);
    splitter->addWidget(fileExplorerWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 0);
    splitter->setSizes(QList<int>() << 1000000 << savedSidebarWidth);
  } else {
    splitter->addWidget(fileExplorerWidget);
    splitter->addWidget(editorSideContainer);
    splitter->setStretchFactor(0, 0);
    splitter->setStretchFactor(1, 1);
    splitter->setSizes(QList<int>() << savedSidebarWidth << 1000000);
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
  connect(workspaceCoordinator, &WorkspaceCoordinator::themeChanged, this,
          &MainWindow::applyTheme);

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

  // TODO: Rework the tab update system to not rely on mass setting
  // all the tabs and have the TabBarWidget be in charge of mgmt/updates,
  // with each TabWidget in control of its repaints?
}

void MainWindow::setupKeyboardShortcuts() {
  auto rustShortcuts = shortcutsManager->get_shortcuts();
  std::unordered_map<std::string, std::string> shortcutMap;
  shortcutMap.reserve(rustShortcuts.size());

  for (const auto &shortcut : rustShortcuts) {
    shortcutMap.emplace(
        std::string(static_cast<rust::String>(shortcut.key).c_str()),
        std::string(static_cast<rust::String>(shortcut.key_combo).c_str()));
  }

  rust::Vec<neko::Shortcut> missingShortcuts;
  auto ensureShortcut = [&](const std::string &key,
                            const std::string &keyCombo) {
    auto it = shortcutMap.find(key);
    if (it == shortcutMap.end() || it->second.empty()) {
      neko::Shortcut shortcut;
      shortcut.key = key;
      shortcut.key_combo = keyCombo;
      missingShortcuts.push_back(std::move(shortcut));
      shortcutMap[key] = keyCombo;
    }
  };

  for (const auto &shortcut : rustShortcuts) {
    ensureShortcut(shortcut.key.data(), shortcut.key_combo.data());
  }

  if (!missingShortcuts.empty()) {
    shortcutsManager->add_shortcuts(std::move(missingShortcuts));
  }

  auto seqFor = [&](const std::string &key,
                    const QKeySequence &fallback) -> QKeySequence {
    auto it = shortcutMap.find(key);
    if (it != shortcutMap.end() && !it->second.empty()) {
      return QKeySequence(it->second.c_str());
    }
    return fallback;
  };

  // Cmd+S for save
  QAction *saveAction = new QAction(this);
  addShortcut(
      saveAction,
      seqFor("Tab::Save", QKeySequence(Qt::ControlModifier | Qt::Key_S)),
      Qt::WindowShortcut, [this]() { workspaceCoordinator->fileSaved(false); });

  // Cmd+Shift+S for save as
  QAction *saveAsAction = new QAction(this);
  addShortcut(
      saveAsAction,
      seqFor("Tab::SaveAs",
             QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_S)),
      Qt::WindowShortcut, [this]() { workspaceCoordinator->fileSaved(true); });

  // Cmd+T for new tab
  QAction *newTabAction = new QAction(this);
  addShortcut(newTabAction,
              seqFor("Tab::New", QKeySequence(Qt::ControlModifier | Qt::Key_T)),
              Qt::WindowShortcut, [this]() { workspaceCoordinator->newTab(); });

  // Cmd+W for close tab
  QAction *closeTabAction = new QAction(this);
  addShortcut(
      closeTabAction,
      seqFor("Tab::Close", QKeySequence(Qt::ControlModifier | Qt::Key_W)),
      Qt::WindowShortcut, [this]() {
        workspaceCoordinator->closeTab(tabController->getActiveTabId(), false);
      });

  // Cmd+Shift+W for force close tab (bypass unsaved confirmation)
  QAction *forceCloseTabAction = new QAction(this);
  addShortcut(forceCloseTabAction,
              seqFor("Tab::ForceClose",
                     QKeySequence(QKeySequence(Qt::ControlModifier,
                                               Qt::ShiftModifier, Qt::Key_W))),
              Qt::WindowShortcut, [this]() {
                workspaceCoordinator->closeTab(tabController->getActiveTabId(),
                                               true);
              });

  // Cmd+Tab for next tab
  QAction *nextTabAction = new QAction(this);
  addShortcut(nextTabAction,
              seqFor("Tab::Next", QKeySequence(Qt::MetaModifier | Qt::Key_Tab)),
              Qt::WindowShortcut, [this]() {
                const int tabCount = tabController->getTabCount();

                if (tabCount <= 0)
                  return;

                const int currentId = tabController->getActiveTabId();

                int currentIndex = -1;
                for (int i = 0; i < tabCount; ++i) {
                  if (tabController->getTabId(i) == currentId) {
                    currentIndex = i;
                    break;
                  }
                }

                if (currentIndex == -1)
                  return;

                const int nextIndex = (currentIndex + 1) % tabCount;
                workspaceCoordinator->tabChanged(
                    tabController->getTabId(nextIndex));
              });

  // Cmd+Shift+Tab for previous tab
  QAction *prevTabAction = new QAction(this);
  addShortcut(
      prevTabAction,
      seqFor("Tab::Previous",
             QKeySequence(Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_Tab)),
      Qt::WindowShortcut, [this]() {
        const int tabCount = tabController->getTabCount();

        if (tabCount <= 0)
          return;

        const int currentId = tabController->getActiveTabId();

        int currentIndex = -1;
        for (int i = 0; i < tabCount; ++i) {
          if (tabController->getTabId(i) == currentId) {
            currentIndex = i;
            break;
          }
        }
        if (currentIndex == -1)
          return;

        const int prevIndex =
            (currentIndex == 0) ? (tabCount - 1) : (currentIndex - 1);
        workspaceCoordinator->tabChanged(tabController->getTabId(prevIndex));
      });

  // Ctrl+G for jump to
  QAction *jumpToAction = new QAction(this);
  addShortcut(
      jumpToAction,
      seqFor("Cursor::JumpTo", QKeySequence(Qt::ControlModifier | Qt::Key_G)),
      Qt::WindowShortcut,
      [this]() { workspaceCoordinator->cursorPositionClicked(); });

  // Ctrl + E for File Explorer toggle
  QAction *toggleExplorerAction = new QAction(this);
  addShortcut(toggleExplorerAction,
              seqFor("FileExplorer::Toggle",
                     QKeySequence(Qt::ControlModifier | Qt::Key_E)),
              Qt::WindowShortcut,
              [this]() { workspaceCoordinator->fileExplorerToggled(); });

  // Ctrl + H for focus File Explorer
  // TODO: Generalize this, i.e. "focus left widget/pane"
  QAction *focusExplorerAction = new QAction(this);
  addShortcut(
      focusExplorerAction,
      seqFor("FileExplorer::Focus", QKeySequence(Qt::MetaModifier | Qt::Key_H)),
      Qt::WindowShortcut, [this]() { fileExplorerWidget->setFocus(); });

  // Ctrl + L for focus Editor
  // TODO: Generalize this, i.e. "focus right widget/pane"
  QAction *focusEditorAction = new QAction(this);
  addShortcut(
      focusEditorAction,
      seqFor("Editor::Focus", QKeySequence(Qt::MetaModifier | Qt::Key_L)),
      Qt::WindowShortcut, [this]() { editorWidget->setFocus(); });

  // Ctrl + , for open config
  QAction *openConfigAction = new QAction(this);
  addShortcut(openConfigAction,
              seqFor("Editor::OpenConfig",
                     QKeySequence(Qt::ControlModifier | Qt::Key_Comma)),
              Qt::WindowShortcut,
              [this]() { workspaceCoordinator->openConfig(); });

  // Ctrl + P for show command palette
  QAction *showCommandPalette = new QAction(this);
  addShortcut(showCommandPalette,
              seqFor("CommandPalette::Show",
                     QKeySequence(Qt::ControlModifier | Qt::Key_P)),
              Qt::WindowShortcut,
              [this]() { commandPaletteWidget->showPalette(); });
}

void MainWindow::applyTheme(const std::string &themeName) {
  std::string targetTheme = themeName;

  if (targetTheme.empty()) {
    return;
  }

  themeManager->set_theme(targetTheme);

  if (titleBarWidget)
    titleBarWidget->applyTheme();
  if (fileExplorerWidget)
    fileExplorerWidget->applyTheme();
  if (editorWidget)
    editorWidget->applyTheme();
  if (gutterWidget)
    gutterWidget->applyTheme();
  if (statusBarWidget)
    statusBarWidget->applyTheme();
  if (tabBarWidget)
    tabBarWidget->applyTheme();
  if (commandPaletteWidget)
    commandPaletteWidget->applyTheme();

  if (newTabButton) {
    QString newTabButtonBackgroundColor =
        UiUtils::getThemeColor(*themeManager, "ui.background");
    QString newTabButtonForegroundColor =
        UiUtils::getThemeColor(*themeManager, "ui.foreground");
    QString newTabButtonBorderColor =
        UiUtils::getThemeColor(*themeManager, "ui.border");
    QString newTabButtonHoverBackgroundColor =
        UiUtils::getThemeColor(*themeManager, "ui.background.hover");

    QString newTabButtonStylesheet =
        QString("QPushButton {"
                "  background: %1;"
                "  color: %2;"
                "  border: none;"
                "  border-left: 1px solid %3;"
                "  border-bottom: 1px solid %3;"
                "  font-size: 20px;"
                "}"
                "QPushButton:hover {"
                "  background: %4;"
                "}")
            .arg(newTabButtonBackgroundColor, newTabButtonForegroundColor,
                 newTabButtonBorderColor, newTabButtonHoverBackgroundColor);
    newTabButton->setStyleSheet(newTabButtonStylesheet);
  }

  if (emptyStateWidget) {
    QString accentMutedColor =
        UiUtils::getThemeColor(*themeManager, "ui.accent.muted");
    QString foregroundColor =
        UiUtils::getThemeColor(*themeManager, "ui.foreground");
    QString emptyStateBackgroundColor =
        UiUtils::getThemeColor(*themeManager, "ui.background");

    QString emptyStateStylesheet =
        QString("QWidget { background-color: %1; }"
                "QPushButton { background-color: %2; border-radius: 4px; "
                "color: %3; }")
            .arg(emptyStateBackgroundColor, accentMutedColor, foregroundColor);
    emptyStateWidget->setStyleSheet(emptyStateStylesheet);
  }

  if (mainSplitter) {
    QString borderColor = UiUtils::getThemeColor(*themeManager, "ui.border");
    QString splitterStylesheet = QString("QSplitter::handle {"
                                         "  background-color: %1;"
                                         "  margin: 0px;"
                                         "}")
                                     .arg(borderColor);
    mainSplitter->setStyleSheet(splitterStylesheet);
  }

  update();
}

template <typename Slot>
void MainWindow::addShortcut(QAction *action, const QKeySequence &sequence,
                             Qt::ShortcutContext context, Slot &&slot) {
  action->setShortcut(sequence);
  action->setShortcutContext(context);
  connect(action, &QAction::triggered, this, std::forward<Slot>(slot));
  addAction(action);
}
