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
  connect(editorWidget, &EditorWidget::saveRequested, this,
          &MainWindow::onFileSaved);
  connect(editorWidget, &EditorWidget::newTabRequested, this,
          &MainWindow::onNewTabRequested);
  connect(editorWidget, &EditorWidget::closeTabRequested, this,
          &MainWindow::onActiveTabCloseRequested);

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

  editorWidget->setFocus();
}

MainWindow::~MainWindow() {
  if (appState) {
    neko_app_state_free(appState);
  }
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
}

void MainWindow::onNewTabRequested() {
  neko_app_state_new_tab(appState);
  updateTabBar();
  switchToActiveTab();
}

void MainWindow::switchToActiveTab() {
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
    editorWidget->setFocus();
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

  tabBarWidget->setTabs(tabTitles);
  tabBarWidget->setCurrentIndex(neko_app_state_get_active_tab_index(appState));

  neko_app_state_free_tab_titles(titles, count);
}

void MainWindow::onFileSelected(const std::string filePath) {
  neko_app_state_new_tab(appState);
  neko_app_state_open_file(appState, filePath.c_str());
  updateTabBar();
  switchToActiveTab();

  editorWidget->updateDimensionsAndRepaint();
  editorWidget->setFocus();
  gutterWidget->updateDimensionsAndRepaint();
}

void MainWindow::onFileSaved(bool isSaveAs) {
  if (isSaveAs) {
    saveAs();
  } else {
    if (neko_app_state_save_file(appState)) {
      // Save successful
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
    } else {
      qDebug() << "Save as failed";
    }
  }
}
