#include "main_window.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include <neko_core.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  EditorWidget *editorWidget = new EditorWidget();
  FileExplorerWidget *fileExplorerWidget = new FileExplorerWidget();
  setCentralWidget(editorWidget);
}

MainWindow::~MainWindow() {}
