#include "main_window.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), appState(neko::new_app_state("")),
      themeManager(neko::new_theme_manager()),
      configManager(neko::new_config_manager()) {
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  neko::Editor *editor = &appState->get_editor_mut();
  neko::FileTree *fileTree = &appState->get_file_tree_mut();

  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(*configManager, *themeManager, this);
  fileExplorerWidget =
      new FileExplorerWidget(fileTree, *configManager, *themeManager, this);
  editorWidget = new EditorWidget(editor, *configManager, *themeManager, this);
  gutterWidget = new GutterWidget(editor, *configManager, *themeManager, this);
  statusBarWidget = new StatusBarWidget(*configManager, *themeManager, this);

  setupKeyboardShortcuts();

  connect(fileExplorerWidget, &FileExplorerWidget::fileSelected, this,
          &MainWindow::onFileSelected);
  connect(fileExplorerWidget, &FileExplorerWidget::directorySelected,
          titleBarWidget, &TitleBarWidget::onDirChanged);
  connect(gutterWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          editorWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(editorWidget->verticalScrollBar(), &QScrollBar::valueChanged,
          gutterWidget->verticalScrollBar(), &QScrollBar::setValue);
  connect(editorWidget, &EditorWidget::fontSizeChanged, gutterWidget,
          &GutterWidget::onEditorFontSizeChanged);
  connect(editorWidget, &EditorWidget::lineCountChanged, gutterWidget,
          &GutterWidget::onEditorLineCountChanged);
  connect(titleBarWidget, &TitleBarWidget::directorySelectionButtonPressed,
          fileExplorerWidget, &FileExplorerWidget::directorySelectionRequested);
  connect(editorWidget, &EditorWidget::cursorPositionChanged, gutterWidget,
          &GutterWidget::onEditorCursorPositionChanged);
  connect(editorWidget, &EditorWidget::newTabRequested, this,
          &MainWindow::onNewTabRequested);
  connect(editorWidget, &EditorWidget::closeTabRequested, this,
          &MainWindow::onActiveTabCloseRequested);
  connect(editorWidget, &EditorWidget::bufferChanged, this,
          &MainWindow::onBufferChanged);
  connect(statusBarWidget, &StatusBarWidget::fileExplorerToggled, this,
          &MainWindow::onFileExplorerToggled);

  QWidget *mainContainer = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(titleBarWidget);

  QWidget *editorSideContainer = new QWidget(this);
  QVBoxLayout *editorSideLayout = new QVBoxLayout(editorSideContainer);
  editorSideLayout->setContentsMargins(0, 0, 0, 0);
  editorSideLayout->setSpacing(0);

  tabBarContainer = new QWidget(this);
  QHBoxLayout *tabBarLayout = new QHBoxLayout(tabBarContainer);
  tabBarLayout->setContentsMargins(0, 0, 0, 0);
  tabBarLayout->setSpacing(0);

  tabBarWidget =
      new TabBarWidget(*configManager, *themeManager, tabBarContainer);
  QPushButton *newTabButton = new QPushButton("+", tabBarContainer);

  QFont uiFont = UiUtils::loadFont(*configManager, neko::FontType::Interface);

  // Height = Font Height + Top Padding (8) + Bottom Padding (8)
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

  connect(tabBarWidget, &TabBarWidget::tabCloseRequested, this,
          &MainWindow::onTabCloseRequested);
  connect(tabBarWidget, &TabBarWidget::currentChanged, this,
          &MainWindow::onTabChanged);
  connect(tabBarWidget, &TabBarWidget::newTabRequested, this,
          &MainWindow::onNewTabRequested);
  connect(newTabButton, &QPushButton::clicked, this,
          &MainWindow::onNewTabRequested);

  editorSideLayout->addWidget(tabBarContainer);

  // Empty state layout
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
  QVBoxLayout *emptyLayout = new QVBoxLayout(emptyStateWidget);
  emptyLayout->setAlignment(Qt::AlignCenter);

  QPushButton *emptyStateNewTabButton =
      new QPushButton("New Tab", emptyStateWidget);
  emptyStateNewTabButton->setFixedSize(80, 35);
  connect(emptyStateNewTabButton, &QPushButton::clicked, this,
          &MainWindow::onNewTabRequested);

  emptyLayout->addWidget(emptyStateNewTabButton);

  // Editor and gutter
  QWidget *editorContainer = new QWidget(this);
  QHBoxLayout *editorLayout = new QHBoxLayout(editorContainer);
  editorLayout->setContentsMargins(0, 0, 0, 0);
  editorLayout->setSpacing(0);
  editorLayout->addWidget(gutterWidget, 0);
  editorLayout->addWidget(editorWidget, 1);
  editorLayout->addWidget(emptyStateWidget);

  emptyStateWidget->hide();

  editorSideLayout->addWidget(editorContainer);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->addWidget(fileExplorerWidget);
  splitter->addWidget(editorSideContainer);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes({250, 600});
  splitter->setHandleWidth(1);

  QString borderColor = UiUtils::getThemeColor(*themeManager, "ui.border");
  QString splitterStylesheet = QString("QSplitter::handle {"
                                       "  background-color: %1;"
                                       "  margin: 0px;"
                                       "}")
                                   .arg(borderColor);
  splitter->setStyleSheet(splitterStylesheet);

  int savedSidebarWidth = configManager->get_file_explorer_width();
  splitter->setSizes(QList<int>() << savedSidebarWidth << 1000000);

  QObject::connect(splitter, &QSplitter::splitterMoved, this,
                   [splitter, this](int pos, int index) {
                     QList<int> sizes = splitter->sizes();

                     if (!sizes.isEmpty()) {
                       int newWidth = sizes.first();

                       configManager->set_file_explorer_width(newWidth);
                     }
                   });

  mainLayout->addWidget(splitter);
  mainLayout->addWidget(statusBarWidget);
  setCentralWidget(mainContainer);

  updateTabBar();

  fileExplorerWidget->loadSavedDir();

  if (!configManager->get_file_explorer_shown()) {
    fileExplorerWidget->hide();
  }

  editorWidget->setFocus();
}

MainWindow::~MainWindow() {}

void MainWindow::onFileExplorerToggled() {
  bool isOpen = fileExplorerWidget->isHidden();

  if (isOpen) {
    fileExplorerWidget->show();
  } else {
    fileExplorerWidget->hide();
  }

  configManager->set_file_explorer_shown(isOpen);
}

void MainWindow::saveCurrentScrollState() {
  if (appState->get_tab_count() > 0) {
    int currentIndex = appState->get_active_tab_index();

    tabScrollOffsets[currentIndex] =
        ScrollOffset{editorWidget->horizontalScrollBar()->value(),
                     editorWidget->verticalScrollBar()->value()};
  }
}

void MainWindow::setupKeyboardShortcuts() {
  // Cmd+S for save
  QAction *saveAction = new QAction(this);
  saveAction->setShortcut(QKeySequence::Save);
  saveAction->setShortcutContext(Qt::WindowShortcut);
  connect(saveAction, &QAction::triggered, this, &MainWindow::onFileSaved);
  addAction(saveAction);

  // Cmd+Shift+S for save as
  QAction *saveAsAction = new QAction(this);
  saveAsAction->setShortcut(QKeySequence::SaveAs);
  saveAsAction->setShortcutContext(Qt::WindowShortcut);
  connect(saveAsAction, &QAction::triggered, this,
          [this]() { onFileSaved(true); });
  addAction(saveAsAction);

  // Cmd+T for new tab
  QAction *newTabAction = new QAction(this);
  newTabAction->setShortcut(QKeySequence::AddTab);
  newTabAction->setShortcutContext(Qt::WindowShortcut);
  connect(newTabAction, &QAction::triggered, this,
          &MainWindow::onNewTabRequested);
  addAction(newTabAction);

  // Cmd+W for close tab
  QAction *closeTabAction = new QAction(this);
  closeTabAction->setShortcut(QKeySequence::Close);
  closeTabAction->setShortcutContext(Qt::WindowShortcut);
  connect(closeTabAction, &QAction::triggered, this,
          &MainWindow::onActiveTabCloseRequested);
  addAction(closeTabAction);

  // Cmd+Tab for next tab
  QAction *nextTabAction = new QAction(this);
  nextTabAction->setShortcut(QKeySequence(Qt::MetaModifier | Qt::Key_Tab));
  nextTabAction->setShortcutContext(Qt::WindowShortcut);
  connect(nextTabAction, &QAction::triggered, this, [this]() {
    int tabCount = appState->get_tab_count();

    if (tabCount > 0) {
      int currentIndex = appState->get_active_tab_index();
      size_t nextIndex = (currentIndex + 1) % tabCount;
      onTabChanged(nextIndex);
    }
  });
  addAction(nextTabAction);

  // Cmd+Shift+Tab for previous tab
  QAction *prevTabAction = new QAction(this);
  prevTabAction->setShortcut(
      QKeySequence(Qt::MetaModifier | Qt::ShiftModifier | Qt::Key_Tab));
  prevTabAction->setShortcutContext(Qt::WindowShortcut);
  connect(prevTabAction, &QAction::triggered, this, [this]() {
    int tabCount = appState->get_tab_count();

    if (tabCount > 0) {
      int currentIndex = appState->get_active_tab_index();
      size_t prevIndex = (currentIndex == 0) ? tabCount - 1 : currentIndex - 1;
      onTabChanged(prevIndex);
    }
  });
  addAction(prevTabAction);
}

void MainWindow::onBufferChanged() {
  int activeIndex = appState->get_active_tab_index();
  bool modified = appState->get_tab_modified(activeIndex);

  tabBarWidget->setTabModified(activeIndex, modified);
}

void MainWindow::onActiveTabCloseRequested() {
  int activeIndex = appState->get_active_tab_index();

  // Save current scroll offset before closing
  saveCurrentScrollState();

  if (appState->close_tab(activeIndex)) {
    // Remove scroll offset for closed tab
    tabScrollOffsets.erase(activeIndex);

    // Shift down indices for tabs after the closed one
    std::unordered_map<int, ScrollOffset> updatedOffsets;
    for (const auto &[tabIndex, offset] : tabScrollOffsets) {
      if (tabIndex > activeIndex) {
        updatedOffsets[tabIndex - 1] = offset;
      } else {
        updatedOffsets[tabIndex] = offset;
      }
    }
    tabScrollOffsets = std::move(updatedOffsets);

    updateTabBar();
    switchToActiveTab();
  }
}

void MainWindow::onTabCloseRequested(int index) {
  saveCurrentScrollState();

  if (appState->close_tab(index)) {
    // Remove scroll offset for closed tab
    tabScrollOffsets.erase(index);

    // Shift down indices for tabs after the closed one
    std::unordered_map<int, ScrollOffset> updatedOffsets;
    for (const auto &[tabIndex, offset] : tabScrollOffsets) {
      if (tabIndex > index) {
        updatedOffsets[tabIndex - 1] = offset;
      } else {
        updatedOffsets[tabIndex] = offset;
      }
    }
    tabScrollOffsets = std::move(updatedOffsets);

    updateTabBar();
    switchToActiveTab();
  }
}

void MainWindow::onTabChanged(int index) {
  saveCurrentScrollState();
  appState->set_active_tab_index(index);
  switchToActiveTab();
  updateTabBar();
}

void MainWindow::onNewTabRequested() {
  saveCurrentScrollState();

  appState->new_tab();
  updateTabBar();
  switchToActiveTab();
}

void MainWindow::switchToActiveTab(bool shouldFocusEditor) {
  if (appState->get_tab_count() == 0) {
    // All tabs closed
    tabBarContainer->hide();
    editorWidget->hide();
    gutterWidget->hide();
    editorWidget->setEditor(nullptr);
    gutterWidget->setEditor(nullptr);
    emptyStateWidget->show();

    fileExplorerWidget->setFocus();
  } else {
    emptyStateWidget->hide();
    tabBarContainer->show();
    editorWidget->show();
    gutterWidget->show();

    neko::Editor &editor = appState->get_editor_mut();
    editorWidget->setEditor(&editor);
    gutterWidget->setEditor(&editor);
    editorWidget->updateDimensionsAndRepaint();
    gutterWidget->updateDimensionsAndRepaint();

    int currentIndex = appState->get_active_tab_index();
    auto it = tabScrollOffsets.find(currentIndex);
    if (it != tabScrollOffsets.end()) {
      editorWidget->horizontalScrollBar()->setValue(it->second.x);
      editorWidget->verticalScrollBar()->setValue(it->second.y);
    } else {
      editorWidget->horizontalScrollBar()->setValue(0);
      editorWidget->verticalScrollBar()->setValue(0);
    }

    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  }
}

void MainWindow::updateTabBar() {
  auto rawTitles = appState->get_tab_titles();
  int count = rawTitles.size();

  QStringList tabTitles;
  for (int i = 0; i < rawTitles.size(); i++) {
    tabTitles.append(QString::fromUtf8(rawTitles[i]));
  }

  rust::Vec<bool> modifieds = appState->get_tab_modified_states();

  tabBarWidget->setTabs(tabTitles, modifieds);
  tabBarWidget->setCurrentIndex(appState->get_active_tab_index());
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

  appState->new_tab();

  if (appState->open_file(filePath)) {
    updateTabBar();
    switchToActiveTab(false);
    editorWidget->updateDimensionsAndRepaint();
    gutterWidget->updateDimensionsAndRepaint();
    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  } else {
    int newTabIndex = appState->get_active_tab_index();
    appState->close_tab(newTabIndex);

    // Clean up scroll offset for the failed tab
    tabScrollOffsets.erase(newTabIndex);

    // Shift down indices for tabs after the closed one
    std::unordered_map<int, ScrollOffset> updatedOffsets;
    for (const auto &[tabIndex, offset] : tabScrollOffsets) {
      if (tabIndex > newTabIndex) {
        updatedOffsets[tabIndex - 1] = offset;
      } else {
        updatedOffsets[tabIndex] = offset;
      }
    }
    tabScrollOffsets = std::move(updatedOffsets);

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

void MainWindow::onFileSaved(bool isSaveAs) {
  if (isSaveAs) {
    saveAs();
  } else {
    if (appState->save_active_file()) {
      // Save successful
      int activeIndex = appState->get_active_tab_index();
      tabBarWidget->setTabModified(activeIndex, false);
    } else {
      // Save failed, fallback to save as
      saveAs();
    }
  }
}

void MainWindow::saveAs() {
  QString filePath =
      QFileDialog::getSaveFileName(this, "Save As", QDir::homePath());

  if (!filePath.isEmpty()) {
    if (appState->save_and_set_path(filePath.toStdString())) {
      int activeIndex = appState->get_active_tab_index();
      tabBarWidget->setTabModified(activeIndex, false);
      updateTabBar();
    } else {
      qDebug() << "Save as failed";
    }
  }
}
