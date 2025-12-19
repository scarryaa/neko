#include "main_window.h"
#include "features/status_bar/status_bar_widget.h"
#include "utils/gui_utils.h"

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
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  neko::Editor *editor = &appState->get_editor_mut();
  neko::FileTree *fileTree = &appState->get_file_tree_mut();
  editorController = new EditorController(editor);
  tabController = new TabController(&*appState);

  setupWidgets(editor, fileTree);
  connectSignals();
  setupLayout();
  setupKeyboardShortcuts();
  applyInitialState(editor);
}

MainWindow::~MainWindow() {}

void MainWindow::setupWidgets(neko::Editor *editor, neko::FileTree *fileTree) {
  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(*configManager, *themeManager, this);
  fileExplorerWidget =
      new FileExplorerWidget(fileTree, *configManager, *themeManager, this);
  commandPaletteWidget =
      new CommandPaletteWidget(*themeManager, *configManager, this);
  editorWidget = new EditorWidget(editor, editorController, *configManager,
                                  *themeManager, this);
  gutterWidget = new GutterWidget(editor, editorController, *configManager,
                                  *themeManager, this);
  statusBarWidget =
      new StatusBarWidget(editor, *configManager, *themeManager, this);
  tabBarContainer = new QWidget(this);
  tabBarWidget =
      new TabBarWidget(*configManager, *themeManager, tabBarContainer);

  setActiveEditor(editor);
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

  // TabBarWidget -> MainWindow connections
  connect(tabBarWidget, &TabBarWidget::tabCloseRequested, this,
          &MainWindow::onTabCloseRequested);
  connect(tabBarWidget, &TabBarWidget::currentChanged, this,
          &MainWindow::onTabChanged);
  connect(tabBarWidget, &TabBarWidget::newTabRequested, this,
          &MainWindow::onNewTabRequested);
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

  auto *newTabButton = new QPushButton("+", tabBarContainer);

  QFont uiFont = UiUtils::loadFont(*configManager, neko::FontType::Interface);
  QFontMetrics fm(uiFont);
  int dynamicHeight = fm.height() + 16;

  QString newTabButtonBackgroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.background");
  QString newTabButtonForegroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.foreground");
  QString newTabButtonBorderColor =
      UiUtils::getThemeColor(*themeManager, "ui.border");
  QString newTabButtonHoverBackgroundColor =
      UiUtils::getThemeColor(*themeManager, "ui.background.hover");

  newTabButton->setFixedSize(dynamicHeight, dynamicHeight);
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
  auto fileExplorerRight = configManager->get_file_explorer_right();
  int savedSidebarWidth = configManager->get_file_explorer_width();

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

                       configManager->set_file_explorer_width(newWidth);
                     }
                   });

  return splitter;
}

void MainWindow::applyInitialState(neko::Editor *editor) {
  updateTabBar();
  refreshStatusBarCursor(editor);

  if (!configManager->get_file_explorer_shown()) {
    fileExplorerWidget->hide();
  }

  fileExplorerWidget->loadSavedDir();
  editorController->refresh();
  editorWidget->setFocus();
}

void MainWindow::setActiveEditor(neko::Editor *newEditor) {
  editorController->setEditor(newEditor);
  editorWidget->setEditor(newEditor);
  gutterWidget->setEditor(newEditor);
  statusBarWidget->setEditor(newEditor);
}

void MainWindow::refreshStatusBarCursor(neko::Editor *editor) {
  if (!editor) {
    return;
  }

  auto cursorPosition = editor->get_last_added_cursor();
  int numberOfCursors = editor->get_cursor_positions().size();
  statusBarWidget->updateCursorPosition(cursorPosition.row, cursorPosition.col,
                                        numberOfCursors);
}

void MainWindow::onFileExplorerToggled() {
  bool isOpen = fileExplorerWidget->isHidden();

  if (isOpen) {
    fileExplorerWidget->show();
  } else {
    fileExplorerWidget->hide();
  }

  configManager->set_file_explorer_shown(isOpen);
}

void MainWindow::onCursorPositionClicked() {
  if (appState->get_tab_count() == 0) {
    return;
  }

  auto cursor = appState->get_editor_mut().get_last_added_cursor();
  auto lineCount = appState->get_editor_mut().get_line_count();

  if (lineCount == 0) {
    return;
  }

  auto maxCol = std::max<size_t>(
      1, appState->get_editor_mut().get_line_length(cursor.row));
  auto lastLineMaxCol = std::max<size_t>(
      1, appState->get_editor_mut().get_line_length(lineCount - 1));

  commandPaletteWidget->jumpToRowColumn(cursor.row, cursor.col, maxCol,
                                        lineCount, lastLineMaxCol);
}

// TODO: Create enum
void MainWindow::onCommandPaletteCommand(const QString &command) {
  if (command == "file explorer: toggle") {
    onFileExplorerToggled();

    bool isOpen = !fileExplorerWidget->isHidden();
    emit onFileExplorerToggledViaShortcut(isOpen);
  }
}

void MainWindow::onCommandPaletteGoToPosition(int row, int col) {
  if (appState->get_tab_count() == 0) {
    return;
  }

  editorController->moveTo(row, col, true);
  editorWidget->setFocus();
}

void MainWindow::saveCurrentScrollState() {
  if (appState->get_tab_count() > 0) {
    int currentIndex = appState->get_active_tab_index();

    tabScrollOffsets[currentIndex] =
        ScrollOffset{editorWidget->horizontalScrollBar()->value(),
                     editorWidget->verticalScrollBar()->value()};
  }
}

void MainWindow::removeTabScrollOffset(int closedIndex) {
  tabScrollOffsets.erase(closedIndex);

  std::unordered_map<int, ScrollOffset> updatedOffsets;
  for (const auto &[tabIndex, offset] : tabScrollOffsets) {
    int newIndex = tabIndex > closedIndex ? tabIndex - 1 : tabIndex;
    updatedOffsets[newIndex] = offset;
  }

  tabScrollOffsets = std::move(updatedOffsets);
}

void MainWindow::handleTabClosed(int closedIndex, int numberOfTabsBeforeClose) {
  removeTabScrollOffset(closedIndex);
  statusBarWidget->onTabClosed(numberOfTabsBeforeClose - 1);
  switchToActiveTab();
  updateTabBar();
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

  // Cmd+Tab for next tab
  QAction *nextTabAction = new QAction(this);
  addShortcut(nextTabAction,
              seqFor("Tab::Next", QKeySequence(Qt::MetaModifier | Qt::Key_Tab)),
              Qt::WindowShortcut, [this]() {
                int tabCount = appState->get_tab_count();

                if (tabCount > 0) {
                  int currentIndex = appState->get_active_tab_index();
                  size_t nextIndex = (currentIndex + 1) % tabCount;
                  onTabChanged(nextIndex);
                }
              });

  // Cmd+Shift+Tab for previous tab
  QAction *prevTabAction = new QAction(this);
  addShortcut(
      prevTabAction,
      seqFor("Tab::Previous",
             QKeySequence(Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_Tab)),
      Qt::WindowShortcut, [this]() {
        int tabCount = appState->get_tab_count();

        if (tabCount > 0) {
          int currentIndex = appState->get_active_tab_index();
          size_t prevIndex =
              (currentIndex == 0) ? tabCount - 1 : currentIndex - 1;
          onTabChanged(prevIndex);
        }
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

template <typename Slot>
void MainWindow::addShortcut(QAction *action, const QKeySequence &sequence,
                             Qt::ShortcutContext context, Slot &&slot) {
  action->setShortcut(sequence);
  action->setShortcutContext(context);
  connect(action, &QAction::triggered, this, std::forward<Slot>(slot));
  addAction(action);
}

void MainWindow::onBufferChanged() {
  int activeIndex = appState->get_active_tab_index();
  bool modified = appState->get_tab_modified(activeIndex);

  tabBarWidget->setTabModified(activeIndex, modified);
}

void MainWindow::onActiveTabCloseRequested(int numberOfTabs) {
  auto closeTab = [this](int activeIndex, int numberOfTabs) {
    // Save current scroll offset before closing
    saveCurrentScrollState();

    if (tabController->closeTab(activeIndex)) {
      handleTabClosed(activeIndex, numberOfTabs);
    }
  };

  if (tabController->getTabTitles().empty()) {
    // Close the window
    QApplication::quit();
  } else {
    int activeIndex = tabController->getActiveTabIndex();

    if (appState->get_tab_modified(activeIndex)) {
      const auto titles = tabController->getTabTitles();
      const CloseDecision closeResult =
          showTabCloseConfirmationDialog(activeIndex, titles);

      switch (closeResult) {
      case CloseDecision::Save: {
        auto result = onFileSaved(false);

        if (result == SaveResult::Saved) {
          closeTab(activeIndex, numberOfTabs);
        }

        return;
      }
      case CloseDecision::DontSave:
        closeTab(activeIndex, numberOfTabs);
        return;
      case CloseDecision::Cancel:
        return;
      }
    } else {
      closeTab(activeIndex, numberOfTabs);
    }
  }
}

MainWindow::CloseDecision MainWindow::showTabCloseConfirmationDialog(
    int index, const rust::Vec<rust::String> &titles) {
  QMessageBox box(QMessageBox::Warning, tr("Close Tab"),
                  tr("%1 has unsaved edits.").arg(titles[index]),
                  QMessageBox::NoButton, this->window());

  auto *saveBtn = box.addButton(tr("Save"), QMessageBox::AcceptRole);
  auto *dontSaveBtn =
      box.addButton(tr("Don’t Save"), QMessageBox::DestructiveRole);
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

void MainWindow::onTabCloseRequested(int index, int numberOfTabs) {
  saveCurrentScrollState();

  auto close = [this, index, numberOfTabs]() {
    if (tabController->closeTab(index)) {
      handleTabClosed(index, numberOfTabs);
    }
  };

  if (!appState->get_tab_modified(index)) {
    close();
    return;
  }

  auto titles = tabController->getTabTitles();

  switch (showTabCloseConfirmationDialog(index, titles)) {
  case CloseDecision::Save: {
    SaveResult result = onFileSaved(false);

    if (result == SaveResult::Saved)
      close();

    return;
  }
  case CloseDecision::DontSave:
    close();
    return;
  case CloseDecision::Cancel:
    return;
  }
}

void MainWindow::onTabChanged(int index) {
  saveCurrentScrollState();
  tabController->setActiveTabIndex(index);

  switchToActiveTab();
  updateTabBar();
}

void MainWindow::onNewTabRequested() {
  saveCurrentScrollState();
  tabController->addTab();
  setActiveEditor(&appState->get_editor_mut());

  updateTabBar();
  switchToActiveTab();
}

void MainWindow::switchToActiveTab(bool shouldFocusEditor) {
  if (appState->get_tab_count() == 0) {
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

    neko::Editor &editor = appState->get_editor_mut();
    setActiveEditor(&editor);
    editorWidget->redraw();
    gutterWidget->updateDimensions();

    int currentIndex = appState->get_active_tab_index();
    auto it = tabScrollOffsets.find(currentIndex);
    if (it != tabScrollOffsets.end()) {
      editorWidget->horizontalScrollBar()->setValue(it->second.x);
      editorWidget->verticalScrollBar()->setValue(it->second.y);
    } else {
      editorWidget->horizontalScrollBar()->setValue(0);
      editorWidget->verticalScrollBar()->setValue(0);
    }

    refreshStatusBarCursor(&editor);

    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  }
}

void MainWindow::updateTabBar() {
  auto rawTitles = tabController->getTabTitles();
  int count = rawTitles.size();

  QStringList tabTitles;
  for (int i = 0; i < rawTitles.size(); i++) {
    tabTitles.append(QString::fromUtf8(rawTitles[i]));
  }

  rust::Vec<bool> modifieds = tabController->getTabModifiedStates();

  tabBarWidget->setTabs(tabTitles, modifieds);
  tabBarWidget->setCurrentIndex(tabController->getActiveTabIndex());
}

void MainWindow::onFileSelected(const std::string &filePath,
                                bool shouldFocusEditor) {
  // Check if file is already open
  if (appState->is_file_open(filePath)) {
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

  if (appState->open_file(filePath)) {
    updateTabBar();
    switchToActiveTab(false);

    editorWidget->redraw();
    gutterWidget->updateDimensions();

    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  } else {
    int newTabIndex = appState->get_active_tab_index();
    appState->close_tab(newTabIndex);

    removeTabScrollOffset(newTabIndex);

    updateTabBar();
  }
}

void MainWindow::switchToTabWithFile(const std::string &path) {
  int index = appState->get_tab_index_by_path(path);

  if (index != -1) {
    // Save current scroll offset
    saveCurrentScrollState();

    appState->set_active_tab_index(index);
    switchToActiveTab();
    updateTabBar();
  }
}

void MainWindow::openConfig() {
  auto configPath = configManager->get_config_path();

  if (!configPath.empty()) {
    onFileSelected(configPath.c_str());
  }
}

MainWindow::SaveResult MainWindow::onFileSaved(bool isSaveAs) {
  if (isSaveAs) {
    auto result = saveAs();
    return result;
  }

  if (appState->save_active_file()) {
    int activeIndex = appState->get_active_tab_index();
    tabBarWidget->setTabModified(activeIndex, false);

    return SaveResult::Saved;
  } else {
    // Save failed, fallback to save as
    SaveResult result = saveAs();
    return result;
  }
}

MainWindow::SaveResult MainWindow::saveAs() {
  QString filePath =
      QFileDialog::getSaveFileName(this, tr("Save As"), QDir::homePath());

  if (filePath.isEmpty())
    return SaveResult::Canceled;

  if (appState->save_and_set_path(filePath.toStdString())) {
    int activeIndex = appState->get_active_tab_index();
    tabBarWidget->setTabModified(activeIndex, false);

    updateTabBar();

    return SaveResult::Saved;
  }

  qDebug() << "Save as failed";
  return SaveResult::Failed;
}
