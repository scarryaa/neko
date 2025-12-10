#include "main_window.h"
#include "utils/mac_utils.h"
#include <neko_core.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  appState = neko_app_state_new(nullptr);
  NekoEditor *editor = neko_app_state_get_editor(appState);
  FileTree *fileTree = neko_app_state_get_file_tree(appState);

  emptyStateWidget = new QWidget(this);
  titleBarWidget = new TitleBarWidget(this);
  fileExplorerWidget = new FileExplorerWidget(fileTree, this);
  editorWidget = new EditorWidget(editor, this);
  gutterWidget = new GutterWidget(editor, this);

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

  tabBarWidget = new TabBarWidget(tabBarContainer);
  tabBarContainer->setFixedHeight(32);
  QPushButton *newTabButton = new QPushButton("+", tabBarContainer);
  newTabButton->setFixedSize(32, 32);
  newTabButton->setStyleSheet("QPushButton {"
                              "  background: black;"
                              "  color: #cccccc;"
                              "  border: none;"
                              "  border-left: 1px solid #3c3c3c;"
                              "  border-bottom: 1px solid #3c3c3c;"
                              "  font-size: 20px;"
                              "}"
                              "QPushButton:hover {"
                              "  background: #2c2c2c;"
                              "}");

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
  emptyStateWidget->setStyleSheet(
      "QWidget { background-color: black; }"
      "QPushButton { background-color: #a589d1; border-radius: 4px; }");
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
  splitter->setStyleSheet("QSplitter::handle {"
                          "  background-color: #3c3c3c;"
                          "  margin: 0px;"
                          "}");

  mainLayout->addWidget(splitter);
  setCentralWidget(mainContainer);

  updateTabBar();

  fileExplorerWidget->loadSavedDir();
  editorWidget->setFocus();
}

MainWindow::~MainWindow() {
  if (appState) {
    neko_app_state_free(appState);
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
    size_t tabCount = neko_app_state_get_tab_count(appState);
    if (tabCount > 0) {
      size_t currentIndex = neko_app_state_get_active_tab_index(appState);
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
    size_t tabCount = neko_app_state_get_tab_count(appState);
    if (tabCount > 0) {
      size_t currentIndex = neko_app_state_get_active_tab_index(appState);
      size_t prevIndex = (currentIndex == 0) ? tabCount - 1 : currentIndex - 1;
      onTabChanged(prevIndex);
    }
  });
  addAction(prevTabAction);
}

void MainWindow::onBufferChanged() {
  int activeIndex = neko_app_state_get_active_tab_index(appState);
  bool modified = neko_app_state_get_tab_modified(appState, activeIndex);

  tabBarWidget->setTabModified(activeIndex, modified);
}

void MainWindow::onActiveTabCloseRequested() {
  int activeIndex = neko_app_state_get_active_tab_index(appState);

  if (neko_app_state_close_tab(appState, activeIndex)) {
    updateTabBar();
    switchToActiveTab();
  }
}

void MainWindow::onTabCloseRequested(int index) {
  if (neko_app_state_close_tab(appState, index)) {
    updateTabBar();
    switchToActiveTab();
  }
}

void MainWindow::onTabChanged(int index) {
  neko_app_state_set_active_tab(appState, index);
  switchToActiveTab();
  updateTabBar();
}

void MainWindow::onNewTabRequested() {
  neko_app_state_new_tab(appState);
  updateTabBar();
  switchToActiveTab();
}

void MainWindow::switchToActiveTab(bool shouldFocusEditor) {
  if (neko_app_state_get_tab_count(appState) == 0) {
    // All tabs closed
    tabBarContainer->hide();
    editorWidget->hide();
    gutterWidget->hide();
    emptyStateWidget->show();

    editorWidget->setEditor(nullptr);
    gutterWidget->setEditor(nullptr);
  } else {
    emptyStateWidget->hide();
    tabBarContainer->show();
    editorWidget->show();
    gutterWidget->show();

    NekoEditor *editor = neko_app_state_get_editor(appState);
    editorWidget->setEditor(editor);
    gutterWidget->setEditor(editor);
    editorWidget->updateDimensionsAndRepaint();
    gutterWidget->updateDimensionsAndRepaint();

    if (shouldFocusEditor) {
      editorWidget->setFocus();
    }
  }
}

void MainWindow::updateTabBar() {
  char **titles = nullptr;
  size_t count = 0;

  neko_app_state_get_tab_titles(appState, &titles, &count);

  QStringList tabTitles;
  for (size_t i = 0; i < count; i++) {
    tabTitles.append(QString::fromUtf8(titles[i]));
  }

  bool *modifieds = nullptr;
  neko_app_state_get_tab_modified_states(appState, &modifieds, &count);

  tabBarWidget->setTabs(tabTitles, modifieds);
  tabBarWidget->setCurrentIndex(neko_app_state_get_active_tab_index(appState));

  neko_app_state_free_tab_titles(titles, count);
}

void MainWindow::onFileSelected(const std::string filePath,
                                bool shouldFocusEditor) {
  neko_app_state_new_tab(appState);
  neko_app_state_open_file(appState, filePath.c_str());
  updateTabBar();
  switchToActiveTab(false);

  editorWidget->updateDimensionsAndRepaint();
  gutterWidget->updateDimensionsAndRepaint();

  if (shouldFocusEditor) {
    editorWidget->setFocus();
  }
}

void MainWindow::onFileSaved(bool isSaveAs) {
  if (isSaveAs) {
    saveAs();
  } else {
    if (neko_app_state_save_file(appState)) {
      // Save successful
      int activeIndex = neko_app_state_get_active_tab_index(appState);
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
    if (neko_app_state_save_and_set_path(appState,
                                         filePath.toStdString().c_str())) {
      int activeIndex = neko_app_state_get_active_tab_index(appState);
      tabBarWidget->setTabModified(activeIndex, false);
      updateTabBar();
    } else {
      qDebug() << "Save as failed";
    }
  }
}
