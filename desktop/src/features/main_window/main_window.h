#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/titlebar/titlebar_widget.h"
#include <QMainWindow>
#include <neko_core.h>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void onFileSelected(const std::string);
  void onDirectorySelected(const std::string);

private:
  NekoAppState *appState;
  FileExplorerWidget *fileExplorerWidget;
  EditorWidget *editorWidget;
  GutterWidget *gutterWidget;
  TitleBarWidget *titleBarWidget;
};

#endif // MAIN_WINDOW_H
