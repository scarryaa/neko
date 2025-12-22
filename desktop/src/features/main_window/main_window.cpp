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

  registerProviders();
  registerCommands();
  setupWidgets(editor, fileTree);
  connectSignals();
  setupLayout();

  auto snapshot = configManager->get_config_snapshot();
  auto currentTheme = snapshot.current_theme;
  applyTheme(std::string(currentTheme.c_str()));
  setupKeyboardShortcuts();
  applyInitialState();
}

MainWindow::~MainWindow() {}

void MainWindow::onFileExplorerToggled() {
  bool isOpen = fileExplorerWidget->isHidden();

  if (isOpen) {
    fileExplorerWidget->show();
  } else {
    fileExplorerWidget->hide();
  }

  auto snapshot = configManager->get_config_snapshot();
  snapshot.file_explorer_shown = isOpen;
  configManager->apply_config_snapshot(snapshot);
}

void MainWindow::onCursorPositionClicked() {
  if (tabController->getTabsEmpty()) {
    return;
  }

  neko::CursorPosition cursor = editorController->getLastAddedCursor();
  int lineCount = editorController->getLineCount();

  if (lineCount == 0) {
    return;
  }

  auto maxCol =
      std::max<size_t>(1, editorController->getLineLength(cursor.row));
  auto lastLineMaxCol =
      std::max<size_t>(1, editorController->getLineLength(lineCount - 1));

  commandPaletteWidget->jumpToRowColumn(cursor.row, cursor.col, maxCol,
                                        lineCount, lastLineMaxCol);
}

void MainWindow::onCommandPaletteGoToPosition(int row, int col) {
  if (tabController->getTabsEmpty()) {
    return;
  }

  editorController->moveTo(row, col, true);
  editorWidget->setFocus();
}

void MainWindow::onCommandPaletteCommand(const QString &command) {
  neko::CommandKindFfi kind;
  rust::String argument;

  if (command == "file explorer: toggle") {
    kind = neko::CommandKindFfi::FileExplorerToggle;
  } else if (command == "set theme: light") {
    kind = neko::CommandKindFfi::ChangeTheme;
    argument = rust::String("Default Light");
  } else if (command == "set theme: dark") {
    kind = neko::CommandKindFfi::ChangeTheme;
    argument = rust::String("Default Dark");
  } else {
    return;
  }

  auto commandFfi = neko::new_command(kind, std::move(argument));
  auto result =
      neko::execute_command(commandFfi, *configManager, *themeManager);

  for (auto &intent : result.intents) {
    switch (intent.kind) {
    case neko::UiIntentKindFfi::ToggleFileExplorer:
      onFileExplorerToggled();
      emit onFileExplorerToggledViaShortcut(!fileExplorerWidget->isHidden());
      break;
    case neko::UiIntentKindFfi::ApplyTheme:
      applyTheme(std::string(intent.argument.c_str()));
      break;
    }
  }
}

void MainWindow::onFileSelected(const std::string &filePath,
                                bool shouldFocusEditor) {
  // Check if file is already open
  if (tabController->getTabWithPathExists(filePath)) {
    // Save current scroll offset before switching
    saveCurrentScrollState();

    // Switch to existing tab instead
    switchToTabWithFile(filePath);
    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }

    return;
  }

  // Check if file exists and is readable before creating tab
  if (!std::filesystem::exists(filePath) ||
      !std::filesystem::is_regular_file(filePath)) {
    return;
  }

  // Save current scroll offset
  saveCurrentScrollState();

  tabController->addTab();

  if (appStateController->openFile(filePath)) {
    updateTabBar();
    switchToActiveTab(false);

    editorWidget->updateDimensions();
    editorWidget->redraw();
    gutterWidget->updateDimensions();
    gutterWidget->redraw();

    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  } else {
    int newTabId = tabController->getActiveTabId();
    tabController->closeTab(newTabId);

    removeTabScrollOffset(newTabId);

    updateTabBar();
  }
}

MainWindow::SaveResult MainWindow::onFileSaved(bool isSaveAs) {
  if (isSaveAs) {
    auto result = saveAs();
    return result;
  }

  if (tabController->saveActiveTab()) {
    int activeId = tabController->getActiveTabId();
    tabBarWidget->setTabModified(activeId, false);

    return SaveResult::Saved;
  } else {
    // Save failed, fallback to save as
    SaveResult result = saveAs();
    return result;
  }
}

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

  setActiveEditor(editor);
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

  connect(newTabButton, &QPushButton::clicked, this,
          &MainWindow::onNewTabRequested);

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

  auto *emptyStateNewTabButton = new QPushButton("New Tab", emptyStateWidget);
  emptyStateNewTabButton->setFixedSize(80, 35);
  connect(emptyStateNewTabButton, &QPushButton::clicked, this,
          &MainWindow::onNewTabRequested);

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
  // GutterWidget <-> EditorWidget
  connect(gutterWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          editorWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(editorWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          gutterWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(editorWidget, &EditorWidget::fontSizeChanged, gutterWidget,
          &GutterWidget::onEditorFontSizeChanged);

  // FileExplorerWidget -> MainWindow
  connect(fileExplorerWidget, &FileExplorerWidget::fileSelected, this,
          &MainWindow::onFileSelected);

  // FileExplorerWidget <-> TitleBarWidget
  connect(fileExplorerWidget, &FileExplorerWidget::directorySelected,
          titleBarWidget, &TitleBarWidget::onDirChanged);

  // TitleBarWidget -> FileExplorerWidget
  connect(titleBarWidget, &TitleBarWidget::directorySelectionButtonPressed,
          fileExplorerWidget, &FileExplorerWidget::directorySelectionRequested);

  // EditorWidget -> MainWindow
  connect(editorWidget, &EditorWidget::newTabRequested, this,
          &MainWindow::onNewTabRequested);

  // EditorController -> MainWindow
  connect(editorController, &EditorController::bufferChanged, this,
          &MainWindow::onBufferChanged);

  // StatusBarWidget -> MainWindow
  connect(statusBarWidget, &StatusBarWidget::fileExplorerToggled, this,
          &MainWindow::onFileExplorerToggled);
  connect(this, &MainWindow::onFileExplorerToggledViaShortcut, statusBarWidget,
          &StatusBarWidget::onFileExplorerToggledExternally);
  connect(statusBarWidget, &StatusBarWidget::cursorPositionClicked, this,
          &MainWindow::onCursorPositionClicked);

  // CommandPalette -> MainWindow
  connect(commandPaletteWidget, &CommandPaletteWidget::goToPositionRequested,
          this, &MainWindow::onCommandPaletteGoToPosition);
  connect(commandPaletteWidget, &CommandPaletteWidget::commandRequested, this,
          &MainWindow::onCommandPaletteCommand);

  // EditorController -> StatusBarWidget
  connect(editorController, &EditorController::cursorChanged, statusBarWidget,
          &StatusBarWidget::onCursorPositionChanged);

  // TabController -> MainWindow
  connect(tabController, &TabController::tabListChanged, this,
          &MainWindow::updateTabBar);
  connect(tabController, &TabController::activeTabChanged, this,
          &MainWindow::switchToActiveTab);

  // TODO: Rework the tab update system to not rely on mass setting
  // all the tabs and have the TabBarWidget be in charge of mgmt/updates,
  // with each TabWidget in control of its repaints?
  // TabBarWidget -> MainWindow connections
  connect(tabBarWidget, &TabBarWidget::tabCloseRequested, this,
          &MainWindow::onTabCloseRequested);
  connect(tabBarWidget, &TabBarWidget::currentChanged, this,
          &MainWindow::onTabChanged);
  connect(tabBarWidget, &TabBarWidget::tabPinnedChanged, this,
          &MainWindow::onTabChanged);
  connect(tabBarWidget, &TabBarWidget::tabUnpinRequested, this,
          &MainWindow::onTabUnpinRequested);
  connect(tabBarWidget, &TabBarWidget::newTabRequested, this,
          &MainWindow::onNewTabRequested);
}

void MainWindow::applyInitialState() {
  updateTabBar();
  refreshStatusBarCursor();

  auto snapshot = configManager->get_config_snapshot();
  if (!snapshot.file_explorer_shown) {
    fileExplorerWidget->hide();
  }

  fileExplorerWidget->loadSavedDir();
  editorController->refresh();
  editorWidget->setFocus();
}

MainWindow::CloseDecision MainWindow::showTabCloseConfirmationDialog(int id) {
  const QString tabTitle = tabController->getTabTitle(id);

  QMessageBox box(QMessageBox::Warning, tr("Close Tab"),
                  tr("%1 has unsaved edits.").arg(tabTitle),
                  QMessageBox::NoButton, this->window());

  auto *saveBtn = box.addButton(tr("Save"), QMessageBox::AcceptRole);
  auto *dontSaveBtn =
      box.addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
  auto *cancelBtn = box.addButton(QMessageBox::Cancel);

  box.setDefaultButton(cancelBtn);
  box.setEscapeButton(cancelBtn);

  box.exec();

  if (box.clickedButton() == saveBtn)
    return CloseDecision::Save;
  if (box.clickedButton() == dontSaveBtn)
    return CloseDecision::DontSave;

  return CloseDecision::Cancel;
}

void MainWindow::setActiveEditor(neko::Editor *newEditor) {
  editorController->setEditor(newEditor);

  if (!newEditor) {
    editorWidget->setEditorController(nullptr);
    gutterWidget->setEditorController(nullptr);
  } else {
    editorWidget->setEditorController(editorController);
    gutterWidget->setEditorController(editorController);
  }
}

void MainWindow::refreshStatusBarCursor() {
  if (!editorController) {
    return;
  }

  auto cursorPosition = editorController->getLastAddedCursor();
  int numberOfCursors = editorController->getCursorPositions().size();
  statusBarWidget->updateCursorPosition(cursorPosition.row, cursorPosition.col,
                                        numberOfCursors);
}

MainWindow::SaveResult MainWindow::saveAs() {
  QString filePath =
      QFileDialog::getSaveFileName(this, tr("Save As"), QDir::homePath());

  if (filePath.isEmpty())
    return SaveResult::Canceled;

  if (tabController->saveActiveTabAndSetPath(filePath.toStdString())) {
    int activeId = tabController->getActiveTabId();
    tabBarWidget->setTabModified(activeId, false);

    updateTabBar();

    return SaveResult::Saved;
  }

  qDebug() << "Save as failed";
  return SaveResult::Failed;
}

void MainWindow::updateTabBar() {
  auto rawTitles = tabController->getTabTitles();
  int count = rawTitles.size();

  QStringList tabTitles;
  QStringList tabPaths;
  for (int i = 0; i < rawTitles.size(); i++) {
    tabTitles.append(QString::fromUtf8(rawTitles[i]));

    const int id = tabController->getTabId(i);
    auto path = tabController->getTabPath(id);
    tabPaths.append(path);
  }

  rust::Vec<bool> modifieds = tabController->getTabModifiedStates();
  rust::Vec<bool> pinnedStates = tabController->getTabPinnedStates();

  tabBarWidget->setTabs(tabTitles, tabPaths, modifieds, pinnedStates);
  tabBarWidget->setCurrentId(tabController->getActiveTabId());

  if (rawTitles.size() != 0) {
    setActiveEditor(&appStateController->getActiveEditorMut());
  } else {
    setActiveEditor(nullptr);
  }
}

void MainWindow::removeTabScrollOffset(int closedId) {
  tabScrollOffsets.erase(closedId);
}

void MainWindow::handleTabClosed(int closedId, int numberOfTabsBeforeClose) {
  removeTabScrollOffset(closedId);
  statusBarWidget->onTabClosed(numberOfTabsBeforeClose - 1);
  switchToActiveTab();
  updateTabBar();
}

void MainWindow::onTabCloseRequested(int id, bool forceClose) {
  if (tabController->getIsPinned(id)) {
    return;
  }

  saveCurrentScrollState();
  tabController->closeTab(id);
}

void MainWindow::onTabChanged(int id) {
  saveCurrentScrollState();
  tabController->setActiveTab(id);

  switchToActiveTab();
  updateTabBar();
}

void MainWindow::onTabUnpinRequested(int id) {
  tabController->unpinTab(id);
  updateTabBar();
}

void MainWindow::onNewTabRequested() {
  saveCurrentScrollState();

  tabController->addTab();
  setActiveEditor(&appStateController->getActiveEditorMut());

  updateTabBar();
  switchToActiveTab();
}

void MainWindow::switchToActiveTab(bool shouldFocusEditor) {
  if (tabController->getTabsEmpty()) {
    // All tabs closed
    tabBarContainer->hide();
    editorWidget->hide();
    gutterWidget->hide();
    setActiveEditor(nullptr);
    emptyStateWidget->show();

    fileExplorerWidget->setFocus();
  } else {
    emptyStateWidget->hide();
    tabBarContainer->show();
    editorWidget->show();
    gutterWidget->show();
    statusBarWidget->showCursorPositionInfo();

    neko::Editor &editor = appStateController->getActiveEditorMut();
    setActiveEditor(&editor);
    editorWidget->updateDimensions();
    editorWidget->redraw();
    gutterWidget->updateDimensions();
    gutterWidget->redraw();

    int currentId = tabController->getActiveTabId();

    auto it = tabScrollOffsets.find(currentId);
    if (it != tabScrollOffsets.end()) {
      editorWidget->horizontalScrollBar()->setValue(it->second.x);
      editorWidget->verticalScrollBar()->setValue(it->second.y);
    } else {
      editorWidget->horizontalScrollBar()->setValue(0);
      editorWidget->verticalScrollBar()->setValue(0);
    }

    refreshStatusBarCursor();

    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  }
}

void MainWindow::onActiveTabCloseRequested(int numberOfTabs,
                                           bool bypassConfirmation) {
  auto closeTab = [this](int activeId, int numberOfTabs) {
    // Save current scroll offset before closing
    saveCurrentScrollState();

    if (tabController->closeTab(activeId)) {
      handleTabClosed(activeId, numberOfTabs);
    }
  };

  if (tabController->getTabCount() == 0) {
    // Close the window
    QApplication::quit();
  } else {
    int activeId = tabController->getActiveTabId();

    if (tabController->getTabModified(activeId) && !bypassConfirmation) {
      const CloseDecision closeResult =
          showTabCloseConfirmationDialog(activeId);

      switch (closeResult) {
      case CloseDecision::Save: {
        auto result = onFileSaved(false);

        if (result == SaveResult::Saved) {
          closeTab(activeId, numberOfTabs);
        }

        return;
      }
      case CloseDecision::DontSave:
        closeTab(activeId, numberOfTabs);
        return;
      case CloseDecision::Cancel:
        return;
      }
    } else {
      closeTab(activeId, numberOfTabs);
    }
  }
}

void MainWindow::onTabCloseOthers(int id, bool forceClose) {
  saveCurrentScrollState();
  tabController->closeOtherTabs(id);
}

void MainWindow::onTabCloseLeft(int id, bool forceClose) {
  saveCurrentScrollState();
  tabController->closeLeftTabs(id);
}

void MainWindow::onTabCloseRight(int id, bool forceClose) {
  saveCurrentScrollState();
  tabController->closeRightTabs(id);
}

void MainWindow::onTabCopyPath(int id) {
  const QString path = tabController->getTabPath(id);

  if (path.isEmpty())
    return;

  QApplication::clipboard()->setText(path);
}

void MainWindow::onTabReveal(int id) {
  const QString path = tabController->getTabPath(id);

  if (path.isEmpty())
    return;

  fileExplorerWidget->showItem(path);
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
      Qt::WindowShortcut, &MainWindow::onFileSaved);

  // Cmd+Shift+S for save as
  QAction *saveAsAction = new QAction(this);
  addShortcut(
      saveAsAction,
      seqFor("Tab::SaveAs",
             QKeySequence(Qt::ControlModifier | Qt::ShiftModifier | Qt::Key_S)),
      Qt::WindowShortcut, [this]() { onFileSaved(true); });

  // Cmd+T for new tab
  QAction *newTabAction = new QAction(this);
  addShortcut(newTabAction,
              seqFor("Tab::New", QKeySequence(Qt::ControlModifier | Qt::Key_T)),
              Qt::WindowShortcut, &MainWindow::onNewTabRequested);

  // Cmd+W for close tab
  QAction *closeTabAction = new QAction(this);
  addShortcut(
      closeTabAction,
      seqFor("Tab::Close", QKeySequence(Qt::ControlModifier | Qt::Key_W)),
      Qt::WindowShortcut,
      [this]() { onActiveTabCloseRequested(tabBarWidget->getNumberOfTabs()); });

  // Cmd+Shift+W for force close tab (bypass unsaved confirmation)
  QAction *forceCloseTabAction = new QAction(this);
  addShortcut(forceCloseTabAction,
              seqFor("Tab::ForceClose",
                     QKeySequence(QKeySequence(Qt::ControlModifier,
                                               Qt::ShiftModifier, Qt::Key_W))),
              Qt::WindowShortcut, [this]() {
                onActiveTabCloseRequested(tabBarWidget->getNumberOfTabs(),
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
                onTabChanged(tabController->getTabId(nextIndex));
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
        onTabChanged(tabController->getTabId(prevIndex));
      });

  // Ctrl+G for jump to
  QAction *jumpToAction = new QAction(this);
  addShortcut(
      jumpToAction,
      seqFor("Cursor::JumpTo", QKeySequence(Qt::ControlModifier | Qt::Key_G)),
      Qt::WindowShortcut, [this]() { onCursorPositionClicked(); });

  // Ctrl + E for File Explorer toggle
  QAction *toggleExplorerAction = new QAction(this);
  addShortcut(toggleExplorerAction,
              seqFor("FileExplorer::Toggle",
                     QKeySequence(Qt::ControlModifier | Qt::Key_E)),
              Qt::WindowShortcut, [this]() {
                onFileExplorerToggled();

                bool isOpen = !fileExplorerWidget->isHidden();
                emit onFileExplorerToggledViaShortcut(isOpen);
              });

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
              Qt::WindowShortcut, &MainWindow::openConfig);

  // Ctrl + P for show command palette
  QAction *showCommandPalette = new QAction(this);
  addShortcut(showCommandPalette,
              seqFor("CommandPalette::Show",
                     QKeySequence(Qt::ControlModifier | Qt::Key_P)),
              Qt::WindowShortcut,
              [this]() { commandPaletteWidget->showPalette(); });
}

void MainWindow::onBufferChanged() {
  int activeId = tabController->getActiveTabId();
  bool modified = tabController->getTabModified(activeId);

  tabBarWidget->setTabModified(activeId, modified);
}

void MainWindow::switchToTabWithFile(const std::string &path) {
  int index = tabController->getTabIndexByPath(path);
  int id = tabController->getTabId(index);

  if (index != -1) {
    // Save current scroll offset
    saveCurrentScrollState();

    tabController->setActiveTab(id);
    switchToActiveTab();
    updateTabBar();
  }
}

void MainWindow::saveCurrentScrollState() {
  if (tabController->getTabsEmpty()) {
    return;
  }

  int currentId = tabController->getActiveTabId();

  tabScrollOffsets[currentId] =
      ScrollOffset{editorWidget->horizontalScrollBar()->value(),
                   editorWidget->verticalScrollBar()->value()};
}

void MainWindow::openConfig() {
  auto configPath = configManager->get_config_path();

  if (!configPath.empty()) {
    onFileSelected(configPath.c_str());
  }
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

void MainWindow::registerProviders() {
  // TODO: Centralize this
  contextMenuRegistry.registerProvider("tab", [](const QVariant &v) {
    const auto ctx = v.value<TabContext>();

    QVector<ContextMenuItem> items;

    items.push_back({ContextMenuItemKind::Action, "tab.close", "Close",
                     "Ctrl+W", "", true});
    items.push_back({ContextMenuItemKind::Action, "tab.closeOthers",
                     "Close Others", "", "", ctx.canCloseOthers});
    items.push_back({ContextMenuItemKind::Action, "tab.closeLeft",
                     "Close Tabs to the Left", "", "", true});
    items.push_back({ContextMenuItemKind::Action, "tab.closeRight",
                     "Close Tabs to the Right", "", "", true});

    items.push_back({ContextMenuItemKind::Separator});

    ContextMenuItem pin;
    pin.kind = ContextMenuItemKind::Action;
    pin.id = "tab.pin";
    pin.label = ctx.isPinned ? "Unpin" : "Pin";
    pin.enabled = true;
    pin.checked = ctx.isPinned;
    items.push_back(pin);

    items.push_back({ContextMenuItemKind::Separator});

    items.push_back({ContextMenuItemKind::Action, "tab.copyPath", "Copy Path",
                     "", "", !ctx.filePath.isEmpty()});
    items.push_back({ContextMenuItemKind::Action, "tab.reveal",
                     "Reveal in Explorer", "", "", !ctx.filePath.isEmpty()});

    return items;
  });
}

void MainWindow::registerCommands() {
  commandRegistry.registerCommand("tab.close", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    auto shiftPressed = QApplication::keyboardModifiers().testFlag(
        Qt::KeyboardModifier::ShiftModifier);

    onTabCloseRequested(ctx.tabId, shiftPressed);
  });
  commandRegistry.registerCommand("tab.closeOthers", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    auto shiftPressed = QApplication::keyboardModifiers().testFlag(
        Qt::KeyboardModifier::ShiftModifier);

    onTabCloseOthers(ctx.tabId, shiftPressed);
  });
  commandRegistry.registerCommand("tab.closeLeft", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    auto shiftPressed = QApplication::keyboardModifiers().testFlag(
        Qt::KeyboardModifier::ShiftModifier);

    onTabCloseLeft(ctx.tabId, shiftPressed);
  });
  commandRegistry.registerCommand("tab.closeRight", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    auto shiftPressed = QApplication::keyboardModifiers().testFlag(
        Qt::KeyboardModifier::ShiftModifier);

    onTabCloseRight(ctx.tabId, shiftPressed);
  });
  commandRegistry.registerCommand("tab.copyPath", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    onTabCopyPath(ctx.tabId);
  });
  commandRegistry.registerCommand("tab.reveal", [this](const QVariant &v) {
    auto ctx = v.value<TabContext>();
    onTabReveal(ctx.tabId);
  });
}

template <typename Slot>
void MainWindow::addShortcut(QAction *action, const QKeySequence &sequence,
                             Qt::ShortcutContext context, Slot &&slot) {
  action->setShortcut(sequence);
  action->setShortcutContext(context);
  connect(action, &QAction::triggered, this, std::forward<Slot>(slot));
  addAction(action);
}
