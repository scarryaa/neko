#ifndef WORKSPACE_COORDINATOR_H
#define WORKSPACE_COORDINATOR_H

#include "features/editor/controllers/editor_controller.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/main_window/controllers/workspace_controller.h"
#include "features/main_window/save_result.h"
#include "features/main_window/workspace_ui_handles.h"
#include "features/tabs/controllers/tab_controller.h"
#include <QObject>

class WorkspaceCoordinator : public QObject {
  Q_OBJECT

public:
  explicit WorkspaceCoordinator(WorkspaceController *workspaceController,
                                TabController *tabController,
                                AppStateController *appStateController,
                                EditorController *editorController,
                                neko::ConfigManager *configManager,
                                neko::ThemeManager *themeManager,
                                WorkspaceUiHandles uiHandles,
                                QObject *parent = nullptr);
  ~WorkspaceCoordinator();

  void fileSelected(const std::string &path, bool focusEditor);
  void fileSaved(bool saveAs);

  void fileExplorerToggled();
  void cursorPositionClicked();
  void commandPaletteGoToPosition(int row, int col);
  void commandPaletteCommand(const QString &command);

  void openConfig();

  void tabCopyPath(int id);
  void tabReveal(int id);
  void newTab();
  void tabChanged(int id);
  void tabUnpinned(int id);

  void bufferChanged();

  CloseDecision showTabCloseConfirmationDialog(const QList<int> &ids);
  SaveResult saveTab(int id, bool isSaveAs);

  void closeTab(int id, bool forceClose);
  void closeLeftTabs(int id, bool forceClose);
  void closeRightTabs(int id, bool forceClose);
  void closeOtherTabs(int id, bool forceClose);

  void applyInitialState();

signals:
  void onFileExplorerToggledViaShortcut(bool isOpen);
  void themeChanged(const std::string &themeName);

private:
  void saveScrollOffsetsForActiveTab();
  void restoreScrollOffsetsForActiveTab();
  void refreshUiForActiveTab(bool focusEditor);
  void updateTabBar();
  void handleTabsClosed(const QList<int> &ids);
  void setActiveEditor(neko::Editor *editor);
  void refreshStatusBarCursorInfo();

  WorkspaceController *workspaceController;
  TabController *tabController;
  AppStateController *appStateController;
  EditorController *editorController;
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  WorkspaceUiHandles uiHandles;
};

#endif
