#include "main_window.h"
#include "features/editor/editor_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "utils/mac_utils.h"
#include <QSplitter>
#include <QVBoxLayout>
#include <neko_core.h>
#include <string>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
  setupMacOSTitleBar(this);
  setAttribute(Qt::WA_NativeWindow);
  setAttribute(Qt::WA_LayoutOnEntireRect);

  appState = neko_app_state_new(nullptr);
  NekoEditor *editor = neko_app_state_get_editor(appState);
  FileTree *fileTree = neko_app_state_get_file_tree(appState);

  titleBarWidget = new TitleBarWidget(this);
  fileExplorerWidget = new FileExplorerWidget(fileTree, this);
  editorWidget = new EditorWidget(editor, this);
  gutterWidget = new GutterWidget(editor, this);

  fileExplorerWidget->setFrameShape(QFrame::NoFrame);
  editorWidget->setFrameShape(QFrame::NoFrame);
  gutterWidget->setFrameShape(QFrame::NoFrame);

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

  QWidget *mainContainer = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(mainContainer);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  mainLayout->addWidget(titleBarWidget);

  QWidget *editorContainer = new QWidget(this);
  QHBoxLayout *editorLayout = new QHBoxLayout(editorContainer);
  editorLayout->setContentsMargins(0, 0, 0, 0);
  editorLayout->setSpacing(0);
  editorLayout->addWidget(gutterWidget, 0);
  editorLayout->addWidget(editorWidget, 1);

  auto *splitter = new QSplitter(Qt::Horizontal, this);
  splitter->addWidget(fileExplorerWidget);
  splitter->addWidget(editorContainer);

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
  editorWidget->setFocus();
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
    gutterWidget->updateDimensionsAndRepaint();
  }
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
