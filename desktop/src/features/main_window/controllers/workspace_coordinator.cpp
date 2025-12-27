#include "workspace_coordinator.h"
#include "features/command_palette/command_palette_mode.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/app_config_service.h"
#include "features/main_window/controllers/app_state_controller.h"
#include "features/main_window/controllers/command_executor.h"
#include "features/main_window/controllers/workspace_controller.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QClipboard>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>

WorkspaceCoordinator::WorkspaceCoordinator(
    const WorkspaceCoordinatorProps &props, QObject *parent)
    : workspaceController(props.workspaceController),
      tabController(props.tabController),
      appStateController(props.appStateController),
      appConfigService(props.appConfigService),
      editorController(props.editorController), uiHandles(props.uiHandles),
      commandExecutor(props.commandExecutor), QObject(parent) {
  // TabController -> WorkspaceCoordinator
  connect(tabController, &TabController::activeTabChanged, this,
          &WorkspaceCoordinator::refreshUiForActiveTab);
  connect(tabController, &TabController::allTabsClosed, this,
          [this] { refreshUiForActiveTab(false); });

  // TabController -> TabBarWidget
  connect(tabController, &TabController::tabOpened, uiHandles->tabBarWidget,
          &TabBarWidget::addTab);
  connect(tabController, &TabController::tabClosed, uiHandles->tabBarWidget,
          &TabBarWidget::removeTab);
  connect(tabController, &TabController::tabMoved, uiHandles->tabBarWidget,
          [this](int fromIndex, int toIndex) {
            uiHandles->tabBarWidget->moveTab(fromIndex, toIndex);
            uiHandles->editorWidget->setFocus();
          });
  connect(tabController, &TabController::tabUpdated, uiHandles->tabBarWidget,
          &TabBarWidget::updateTab);
  connect(tabController, &TabController::activeTabChanged,
          uiHandles->tabBarWidget, &TabBarWidget::setCurrentTabId);

  neko::Editor &activeEditor = appStateController->getActiveEditorMut();
  setActiveEditor(&activeEditor);
}

void WorkspaceCoordinator::fileExplorerToggled() {
  const bool shouldShow = uiHandles->fileExplorerWidget->isHidden();
  uiHandles->fileExplorerWidget->setVisible(shouldShow);

  auto snapshot = appConfigService->getSnapshot();
  snapshot.file_explorer_shown = shouldShow;
  appConfigService->setFileExplorerShown(shouldShow);

  emit onFileExplorerToggledViaShortcut(
      !uiHandles->fileExplorerWidget->isHidden());
}

void WorkspaceCoordinator::cursorPositionClicked() {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  neko::CursorPosition cursor = editorController->getLastAddedCursor();
  int lineCount = editorController->getLineCount();

  if (lineCount == 0) {
    return;
  }

  auto maxCol = std::max<size_t>(
      1, editorController->getLineLength(static_cast<int>(cursor.row)));
  auto lastLineMaxCol =
      std::max<size_t>(1, editorController->getLineLength(lineCount - 1));

  uiHandles->commandPaletteWidget->showPalette(
      CommandPaletteMode::Jump,
      {
          .maxLineCount = lineCount,
          .maxColumn = static_cast<int>(maxCol),
          .lastLineMaxColumn = static_cast<int>(lastLineMaxCol),
          .maxRow = lineCount,
          .currentRow = static_cast<int>(cursor.row),
          .currentColumn = static_cast<int>(cursor.col),
      });
}

void WorkspaceCoordinator::commandPaletteGoToPosition(int row, int col) {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  editorController->moveTo(row, col, true);
  uiHandles->editorWidget->setFocus();
}

void WorkspaceCoordinator::commandPaletteCommand(const QString &command) {
  neko::CommandKindFfi kind;
  rust::String argument;

  if (command == "file explorer: toggle") {
    kind = neko::CommandKindFfi::FileExplorerToggle;
  } else if (command == "set theme: light") {
    kind = neko::CommandKindFfi::ChangeTheme;
    argument = rust::String("Default Light");
  } else if (command == "set theme: dark") {
    kind = neko::CommandKindFfi::ChangeTheme;
    argument = rust::String("Default Dark");
  } else {
    return;
  }

  auto result = commandExecutor->execute(kind, std::move(argument));

  for (auto &intent : result.intents) {
    switch (intent.kind) {
    case neko::UiIntentKindFfi::ToggleFileExplorer:
      fileExplorerToggled();
      break;
    case neko::UiIntentKindFfi::ApplyTheme:
      emit themeChanged(std::string(intent.argument.c_str()));
      break;
    }
  }
}

void WorkspaceCoordinator::fileSelected(const std::string &path,
                                        bool focusEditor) {
  // Save scroll offsets if there is an active tab
  {
    const auto snapshot = tabController->getTabsSnapshot();
    if (snapshot.active_present) {
      saveScrollOffsetsForActiveTab();
    }

    // If file already open, just activate it
    for (const auto &tab : snapshot.tabs) {
      if (tab.path_present && tab.path == path) {
        tabController->setActiveTab(static_cast<int>(tab.id));
        return;
      }
    }
  }

  // Otherwise, create a tab, open file into it, rollback on failure
  const int newTabId = tabController->addTab();
  if (!appStateController->openFile(path)) {
    tabController->closeTab(newTabId);
    return;
  }
}

void WorkspaceCoordinator::fileSaved(bool saveAs) {
  const auto snapshot = tabController->getTabsSnapshot();

  if (snapshot.active_present) {
    const int activeId = static_cast<int>(snapshot.active_id);
    const bool success = workspaceController->saveTab(activeId, saveAs);

    if (success) {
      uiHandles->tabBarWidget->setTabModified(activeId, false);
    }
  }
}

void WorkspaceCoordinator::openConfig() {
  auto configPath = appConfigService->getConfigPath();

  if (!configPath.empty()) {
    fileSelected(configPath, true);
  }
}

void WorkspaceCoordinator::handleTabCommand(const std::string &commandId,
                                            const neko::TabContextFfi &ctx,
                                            bool forceClose) {
  if (commandId.empty()) {
    return;
  }
  const int tabId = static_cast<int>(ctx.id);

  if (commandId == "tab.close") {
    closeTab(tabId, forceClose);
  } else if (commandId == "tab.closeOthers") {
    closeOtherTabs(tabId, forceClose);
  } else if (commandId == "tab.closeLeft") {
    closeLeftTabs(tabId, forceClose);
  } else if (commandId == "tab.closeRight") {
    closeRightTabs(tabId, forceClose);
  } else if (commandId == "tab.closeAll") {
    closeAllTabs(forceClose);
  } else if (commandId == "tab.closeClean") {
    closeCleanTabs();
  } else if (commandId == "tab.copyPath") {
    tabCopyPath(tabId);
  } else if (commandId == "tab.reveal") {
    tabReveal("tab.reveal", ctx);
  } else if (commandId == "tab.pin") {
    tabTogglePin(tabId, ctx.is_pinned);
  }
}

void WorkspaceCoordinator::tabCopyPath(int tabId) {
  const auto snapshot = tabController->getTabsSnapshot();
  QString path = QString();
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == tabId) {
      path = QString::fromUtf8(tab.path);
    }
  }

  if (path.isEmpty()) {
    return;
  }

  QApplication::clipboard()->setText(path);
}

void WorkspaceCoordinator::tabTogglePin(int tabId, bool tabIsPinned) {
  if (tabIsPinned) {
    tabController->unpinTab(tabId);
  } else {
    tabController->pinTab(tabId);
  }
}

void WorkspaceCoordinator::tabReveal(const std::string &commandId,
                                     const neko::TabContextFfi &ctx) {
  if (commandId.empty() || !ctx.file_path_present) {
    return;
  }

  appStateController->runTabCommand(commandId, ctx);

  // Show File Explorer if hidden
  if (uiHandles->fileExplorerWidget->isHidden()) {
    fileExplorerToggled();
  }
  emit tabRevealedInFileExplorer();
}

void WorkspaceCoordinator::newTab() {
  saveScrollOffsetsForActiveTab();

  tabController->addTab();
}

void WorkspaceCoordinator::tabChanged(int tabId) {
  saveScrollOffsetsForActiveTab();
  tabController->setActiveTab(tabId);
}

void WorkspaceCoordinator::tabUnpinned(int tabId) {
  tabController->unpinTab(tabId);
}

void WorkspaceCoordinator::bufferChanged() {
  const auto snapshot = tabController->getTabsSnapshot();
  int activeId = static_cast<int>(snapshot.active_id);

  bool modified = false;
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == activeId) {
      modified = tab.modified;
      break;
    }
  }

  uiHandles->tabBarWidget->setTabModified(activeId, modified);
}

CloseDecision
WorkspaceCoordinator::showTabCloseConfirmationDialog(const QList<int> &ids) {
  if (ids.isEmpty()) {
    return CloseDecision::DontSave;
  }

  const auto snapshot = tabController->getTabsSnapshot();

  QSet<int> idSet;
  idSet.reserve(ids.size());
  for (int tabId : ids) {
    idSet.insert(tabId);
  }

  int modifiedCount = 0;
  for (const auto &tab : snapshot.tabs) {
    const int tabId = static_cast<int>(tab.id);

    if (idSet.contains(tabId) && tab.modified) {
      modifiedCount++;
    }
  }

  if (modifiedCount == 0) {
    return CloseDecision::DontSave;
  }

  bool multipleModifiedTabs = modifiedCount > 1;
  QMessageBox box(QMessageBox::Warning, tr("Close Tabs"),
                  tr("%1 tab%2 unsaved edits.")
                      .arg(modifiedCount)
                      .arg(multipleModifiedTabs ? "s have" : " has"),
                  QMessageBox::NoButton, uiHandles->window);

  auto *saveBtn = box.addButton(tr(multipleModifiedTabs ? "Save all" : "Save"),
                                QMessageBox::AcceptRole);
  auto *dontSaveBtn =
      box.addButton(tr(multipleModifiedTabs ? "Discard all" : "Don't Save"),
                    QMessageBox::DestructiveRole);
  auto *cancelBtn = box.addButton(QMessageBox::Cancel);

  box.setDefaultButton(cancelBtn);
  box.setEscapeButton(cancelBtn);

  box.exec();

  if (box.clickedButton() == saveBtn) {
    return CloseDecision::Save;
  }

  if (box.clickedButton() == dontSaveBtn) {
    return CloseDecision::DontSave;
  }

  return CloseDecision::Cancel;
}

std::optional<std::string>
WorkspaceCoordinator::requestFileExplorerDirectory() {
  return workspaceController->requestFileExplorerDirectory();
}

SaveResult WorkspaceCoordinator::saveTab(int tabId, bool isSaveAs) {
  if (workspaceController->saveTab(tabId, isSaveAs)) {
    uiHandles->tabBarWidget->setTabModified(tabId, false);

    return SaveResult::Saved;
  }

  qDebug() << "Save as failed";
  return SaveResult::Failed;
}

void WorkspaceCoordinator::revealActiveTab() {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  neko::TabContextFfi ctx{};
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == snapshot.active_id) {
      ctx.id = tab.id;
      ctx.is_pinned = tab.pinned;
      ctx.is_modified = tab.modified;
      ctx.file_path_present = tab.path_present;
      ctx.file_path = tab.path;
      break;
    }
  }

  handleTabCommand("tab.reveal", ctx, false);
}

void WorkspaceCoordinator::closeTab(int tabId, bool forceClose) {
  auto close = [this](int tabId, bool forceClose) {
    // Save current scroll offset before closing
    saveScrollOffsetsForActiveTab();

    auto ids = workspaceController->closeTab(tabId, forceClose);
    if (!ids.empty()) {
      handleTabsClosed();
    }
  };

  auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    // Close the window
    QApplication::quit();
    return;
  }

  bool isPinned = false;
  for (const auto &tab : snapshot.tabs) {
    if (tabId == tab.id) {
      isPinned = tab.pinned;
    }
  }

  if (isPinned && !forceClose) {
    return;
  }

  close(tabId, forceClose && !isPinned);
}

void WorkspaceCoordinator::closeLeftTabs(int tabId, bool forceClose) {
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeLeft(tabId, forceClose);
  if (!ids.empty()) {
    handleTabsClosed();
  }
}

void WorkspaceCoordinator::closeRightTabs(int tabId, bool forceClose) {
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeRight(tabId, forceClose);
  if (!ids.empty()) {
    handleTabsClosed();
  }
}

void WorkspaceCoordinator::closeAllTabs(bool forceClose) {
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeAll(forceClose);
  if (!ids.empty()) {
    handleTabsClosed();
  }
}

void WorkspaceCoordinator::closeCleanTabs() {
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeClean();
  if (!ids.empty()) {
    handleTabsClosed();
  }
}

void WorkspaceCoordinator::closeOtherTabs(int tabId, bool forceClose) {
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeOthers(tabId, forceClose);

  if (!ids.empty()) {
    handleTabsClosed();
  }
}

void WorkspaceCoordinator::applyInitialState() {
  const auto snapshot = tabController->getTabsSnapshot();

  // NOTE: Not currently needed, might be needed in the future?
  // uiHandles->tabBarWidget->clear();

  int index = 0;
  for (const auto &tab : snapshot.tabs) {
    TabPresentation presentation;
    presentation.id = static_cast<int>(tab.id);
    presentation.title = QString::fromUtf8(tab.title);
    presentation.path = QString::fromUtf8(tab.path);
    presentation.pinned = tab.pinned;
    presentation.modified = tab.modified;

    uiHandles->tabBarWidget->addTab(presentation, index++);
  }

  if (snapshot.active_present) {
    uiHandles->tabBarWidget->setCurrentTabId(
        static_cast<int>(snapshot.active_id));
  }

  refreshStatusBarCursorInfo();

  auto cfg = appConfigService->getSnapshot();
  if (!cfg.file_explorer_shown) {
    uiHandles->fileExplorerWidget->hide();
  }

  editorController->refresh();
  uiHandles->editorWidget->setFocus();
}

void WorkspaceCoordinator::saveScrollOffsetsForActiveTab() {
  const auto snapshot = tabController->getTabsSnapshot();

  if (!snapshot.active_present) {
    return;
  }

  int activeId = static_cast<int>(snapshot.active_id);
  double horizontalScrollOffset =
      uiHandles->editorWidget->horizontalScrollBar()->value();
  double verticalScrollOffset =
      uiHandles->editorWidget->verticalScrollBar()->value();
  neko::ScrollOffsetFfi scrollOffsets = {
      static_cast<int32_t>(horizontalScrollOffset),
      static_cast<int32_t>(verticalScrollOffset)};

  tabController->setTabScrollOffsets(activeId, scrollOffsets);
}

void WorkspaceCoordinator::restoreScrollOffsetsForActiveTab() {
  const auto snapshot = tabController->getTabsSnapshot();

  neko::ScrollOffsetFfi offsets;
  for (const auto &tab : snapshot.tabs) {
    if (tab.id == snapshot.active_id) {
      offsets = tab.scroll_offsets;
    }
  }

  uiHandles->editorWidget->horizontalScrollBar()->setValue(offsets.x);
  uiHandles->editorWidget->verticalScrollBar()->setValue(offsets.y);
}

void WorkspaceCoordinator::refreshUiForActiveTab(bool focusEditor) {
  const auto snapshot = tabController->getTabsSnapshot();

  if (!snapshot.active_present) {
    uiHandles->tabBarContainerWidget->hide();
    uiHandles->editorWidget->hide();
    uiHandles->gutterWidget->hide();
    uiHandles->emptyStateWidget->show();
    uiHandles->fileExplorerWidget->setFocus();

    return;
  }

  // IMPORTANT: set active editor before re-showing widgets, otherwise
  // Qt layout triggers a resize event (which queries the line count with the
  // old editor reference)
  neko::Editor &activeEditor = appStateController->getActiveEditorMut();
  setActiveEditor(&activeEditor);

  uiHandles->emptyStateWidget->hide();
  uiHandles->tabBarContainerWidget->show();
  uiHandles->editorWidget->show();
  uiHandles->gutterWidget->show();
  uiHandles->statusBarWidget->showCursorPositionInfo();

  uiHandles->editorWidget->updateDimensions();
  uiHandles->editorWidget->redraw();
  uiHandles->gutterWidget->updateDimensions();
  uiHandles->gutterWidget->redraw();

  restoreScrollOffsetsForActiveTab();
  refreshStatusBarCursorInfo();

  if (focusEditor) {
    uiHandles->editorWidget->setFocus();
  }
}

void WorkspaceCoordinator::handleTabsClosed() {
  const auto snapshot = tabController->getTabsSnapshot();
  const int newTabCount = static_cast<int>(snapshot.tabs.size());

  uiHandles->statusBarWidget->onTabClosed(newTabCount);
}

void WorkspaceCoordinator::setActiveEditor(neko::Editor *editor) {
  editorController->setEditor(editor);

  uiHandles->editorWidget->setEditorController(editorController);
  uiHandles->gutterWidget->setEditorController(editorController);
}

void WorkspaceCoordinator::refreshStatusBarCursorInfo() {
  if (editorController == nullptr) {
    return;
  }

  auto cursorPosition = editorController->getLastAddedCursor();
  int numberOfCursors =
      static_cast<int>(editorController->getCursorPositions().size());
  uiHandles->statusBarWidget->updateCursorPosition(
      static_cast<int>(cursorPosition.row),
      static_cast<int>(cursorPosition.col), numberOfCursors);
}
