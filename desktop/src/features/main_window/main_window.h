#ifndef MAIN_WINDOW_H
#define MAIN_WINDOW_H

#include "features/command_palette/command_palette_widget.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include "features/title_bar/title_bar_widget.h"
#include "neko-core/src/ffi/mod.rs.h"
#include "utils/gui_utils.h"
#include "utils/mac_utils.h"
#include "utils/scroll_offset.h"
#include <QMainWindow>
#include <QPushButton>
#include <QSplitter>
#include <QVBoxLayout>
#include <string>
#include <unordered_map>

class MainWindow : public QMainWindow {
  Q_OBJECT

private:
  enum class SaveResult { Saved, Canceled, Failed };
  enum class CloseDecision { Save, DontSave, Cancel };

public:
  explicit MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

public slots:
  void onFileExplorerToggled();
  void onCursorPositionClicked();
  void onCommandPaletteGoToPosition(int row, int col);
  void onCommandPaletteCommand(const QString &command);

private slots:
  void onFileSelected(const std::string &filePath,
                      bool shouldFocusEditor = true);
  MainWindow::SaveResult onFileSaved(bool isSaveAs);

signals:
  void onFileExplorerToggledViaShortcut(bool isOpen);
  void onThemeChanged(std::string newTheme);

private:
  void setupWidgets(neko::Editor *editor, neko::FileTree *fileTree);
  void setupLayout();
  QWidget *buildTabBarSection();
  QWidget *buildEmptyStateSection();
  QWidget *buildEditorSection(QWidget *emptyState);
  QSplitter *buildSplitter(QWidget *editorSideContainer);
  void connectSignals();
  void applyInitialState();
  CloseDecision
  showTabCloseConfirmationDialog(int index,
                                 const rust::Vec<rust::String> &titles);

  void setActiveEditor(neko::Editor *newEditor);
  void refreshStatusBarCursor();
  SaveResult saveAs();
  void updateTabBar();
  void removeTabScrollOffset(int closedIndex);
  void handleTabClosed(int closedIndex, int numberOfTabsBeforeClose);
  void onTabCloseRequested(int index, int numberOfTabs,
                           bool bypassConfirmation = false);
  void onTabChanged(int index);
  void onNewTabRequested();
  void switchToActiveTab(bool shouldFocusEditor = true);
  void onActiveTabCloseRequested(int numberOfTabs,
                                 bool bypassConfirmation = false);
  void setupKeyboardShortcuts();
  void onBufferChanged();
  void switchToTabWithFile(const std::string &path);
  void saveCurrentScrollState();
  void openConfig();
  void applyTheme(const std::string &themeName);

  template <typename Slot>
  void addShortcut(QAction *action, const QKeySequence &sequence,
                   Qt::ShortcutContext context, Slot &&slot);

  rust::Box<neko::AppState> appState;
  rust::Box<neko::ThemeManager> themeManager;
  rust::Box<neko::ConfigManager> configManager;
  rust::Box<neko::ShortcutsManager> shortcutsManager;
  EditorController *editorController;
  TabController *tabController;
  AppStateController *appStateController;

  QWidget *emptyStateWidget;
  FileExplorerWidget *fileExplorerWidget;
  CommandPaletteWidget *commandPaletteWidget;
  EditorWidget *editorWidget;
  GutterWidget *gutterWidget;
  TitleBarWidget *titleBarWidget;
  QWidget *tabBarContainer;
  TabBarWidget *tabBarWidget;
  StatusBarWidget *statusBarWidget;
  QPushButton *newTabButton;
  QSplitter *mainSplitter;

  std::unordered_map<int, ScrollOffset> tabScrollOffsets;
};

#endif // MAIN_WINDOW_H
