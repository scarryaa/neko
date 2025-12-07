#include "main_window.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <neko_core.h>
#include <string>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  appState = neko_app_state_new(nullptr);

  NekoEditor *editor = neko_app_state_get_editor(appState);
  FileTree *fileTree = neko_app_state_get_file_tree(appState);

  fileExplorerWidget = new FileExplorerWidget(fileTree, this);
  editorWidget = new EditorWidget(editor, this);

  connect(fileExplorerWidget, &FileExplorerWidget::fileSelected, this,
          &MainWindow::onFileSelected);
  connect(fileExplorerWidget, &FileExplorerWidget::directorySelected, this,
          &MainWindow::onDirectorySelected);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->addWidget(fileExplorerWidget);
  splitter->addWidget(editorWidget);
  splitter->setStretchFactor(0, 0);
  splitter->setStretchFactor(1, 1);
  splitter->setSizes({250, 600});

  splitter->setHandleWidth(1);

  splitter->setStyleSheet("QSplitter::handle {"
                          "  background-color: #3c3c3c;"
                          "  margin: 0px;"
                          "}");

  setCentralWidget(splitter);
}

MainWindow::~MainWindow() {
  if (appState) {
    neko_app_state_free(appState);
  }
}

void MainWindow::onFileSelected(const std::string filePath) {
  if (neko_app_state_open_file(appState, filePath.c_str())) {
    editorWidget->updateDimensionsAndRepaint();
    editorWidget->setFocus();
  }
}

void MainWindow::onDirectorySelected(const std::string directoryPath) {}
