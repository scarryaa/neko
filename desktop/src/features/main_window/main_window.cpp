#include "main_window.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <neko_core.h>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  auto *fileExplorerWidget = new FileExplorerWidget();
  auto *editorWidget = new EditorWidget();

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

MainWindow::~MainWindow() {}
