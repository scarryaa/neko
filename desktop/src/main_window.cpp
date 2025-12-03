#include "main_window.h"
#include "editor_widget.h"
#include <neko_core.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  EditorWidget *editorWidget = new EditorWidget();
  setCentralWidget(editorWidget);
}

MainWindow::~MainWindow() {}
