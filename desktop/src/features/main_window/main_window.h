#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/tabs/tab_bar_widget.h"
#include "features/title_bar/title_bar_widget.h"
#include <QMainWindow>
#include <QSplitter>
#include <QVBoxLayout>
#include <neko_core.h>
#include <string>

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

private slots:
  void onFileSelected(const std::string, bool shouldFocusEditor = true);
  void onFileSaved(bool isSaveAs);

private:
  void saveAs();
  void updateTabBar();
  void onTabCloseRequested(int index);
  void onTabChanged(int index);
  void onNewTabRequested();
  void switchToActiveTab(bool shouldFocusEditor = true);
  void onActiveTabCloseRequested();
  void setupKeyboardShortcuts();
  void onBufferChanged();

  NekoAppState *appState;
  QWidget *emptyStateWidget;
  FileExplorerWidget *fileExplorerWidget;
  EditorWidget *editorWidget;
  GutterWidget *gutterWidget;
  TitleBarWidget *titleBarWidget;
  QWidget *tabBarContainer;
  TabBarWidget *tabBarWidget;
};

#endif // MAIN_WINDOW_H
