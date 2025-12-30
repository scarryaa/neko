#include "workspace_coordinator.h"
#include "features/command_palette/command_palette_widget.h"
#include "features/editor/controllers/editor_controller.h"
#include "features/editor/editor_widget.h"
#include "features/editor/gutter_widget.h"
#include "features/file_explorer/file_explorer_widget.h"
#include "features/main_window/controllers/app_controller.h"
#include "features/main_window/controllers/command_executor.h"
#include "features/main_window/services/app_config_service.h"
#include "features/main_window/services/dialog_service.h"
#include "features/main_window/ui_handles.h"
#include "features/status_bar/status_bar_widget.h"
#include "features/tabs/controllers/tab_controller.h"
#include "features/tabs/tab_bar_widget.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QClipboard>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QScrollBar>

WorkspaceCoordinator::WorkspaceCoordinator(
    const WorkspaceCoordinatorProps &props, QObject *parent)
    : tabController(props.tabController), appController(props.appController),
      appConfigService(props.appConfigService),
      editorController(props.editorController), uiHandles(props.uiHandles),
      commandExecutor(props.commandExecutor),
      tabFlows({.tabController = props.tabController,
                .appController = props.appController,
                .editorController = props.editorController,
                .uiHandles = props.uiHandles}),
      QObject(parent) {
  // TabController -> WorkspaceCoordinator
  connect(tabController, &TabController::activeTabChanged, this,
          &WorkspaceCoordinator::refreshUiForActiveTab);
  connect(tabController, &TabController::allTabsClosed, this,
          [this] { refreshUiForActiveTab(false); });
  connect(tabController, &TabController::restoreScrollOffsetsForReopenedTab,
          this, [this](const TabScrollOffsets &scrollOffsets) {
            tabFlows.restoreScrollOffsetsForReopenedTab(scrollOffsets);
          });

  // TabController -> TabBarWidget
  connect(tabController, &TabController::tabOpened, uiHandles.tabBarWidget,
          &TabBarWidget::addTab);
  connect(tabController, &TabController::tabClosed, uiHandles.tabBarWidget,
          &TabBarWidget::removeTab);
  connect(tabController, &TabController::tabMoved, uiHandles.tabBarWidget,
          [this](int fromIndex, int toIndex) {
            uiHandles.tabBarWidget->moveTab(fromIndex, toIndex);
            uiHandles.editorWidget->setFocus();
          });
  connect(tabController, &TabController::tabUpdated, uiHandles.tabBarWidget,
          &TabBarWidget::updateTab);
  connect(tabController, &TabController::activeTabChanged,
          uiHandles.tabBarWidget, &TabBarWidget::setCurrentTabId);

  // WorkspaceCoordinator -> TabController
  connect(this, &WorkspaceCoordinator::fileOpened, tabController,
          &TabController::fileOpened);

  auto &activeEditor = appController->getActiveEditorMut();
  setActiveEditor(&activeEditor);
}

void WorkspaceCoordinator::fileExplorerToggled() {
  const bool shouldShow = uiHandles.fileExplorerWidget->isHidden();
  uiHandles.fileExplorerWidget->setVisible(shouldShow);

  auto snapshot = appConfigService->getSnapshot();
  snapshot.file_explorer.shown = shouldShow;
  appConfigService->setFileExplorerShown(shouldShow);

  emit onFileExplorerToggledViaShortcut(
      !uiHandles.fileExplorerWidget->isHidden());
}

void WorkspaceCoordinator::cursorPositionClicked() {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  const auto cursor = editorController->getLastAddedCursor();
  const int lineCount = editorController->getLineCount();

  if (lineCount == 0) {
    return;
  }

  const auto maxCol =
      std::max<std::size_t>(1, editorController->getLineLength(cursor.row));
  const auto lastLineMaxCol =
      std::max<std::size_t>(1, editorController->getLineLength(lineCount - 1));

  uiHandles.commandPaletteWidget->showPalette(
      CommandPaletteMode::Jump,
      {
          .maxLineCount = lineCount,
          .maxColumn = static_cast<int>(maxCol),
          .lastLineMaxColumn = static_cast<int>(lastLineMaxCol),
          .maxRow = lineCount,
          .currentRow = cursor.row,
          .currentColumn = cursor.column,
      });
}

std::vector<ShortcutHintRow> WorkspaceCoordinator::buildJumpHintRows() {
  std::vector<ShortcutHintRow> result;
  const auto jumpCommands = AppController::getAvailableJumpCommands();

  result.reserve(jumpCommands.size());
  for (const auto &cmd : jumpCommands) {
    result.push_back({
        QString::fromUtf8(cmd.key),
        QString::fromUtf8(cmd.display_name),
    });
  }

  return result;
}

std::optional<std::string>
WorkspaceCoordinator::requestFileExplorerDirectory() const {
  const QString dir =
      DialogService::openDirectorySelectionDialog(uiHandles.window);
  if (dir.isEmpty()) {
    return std::nullopt;
  }

  return dir.toStdString();
}

void WorkspaceCoordinator::commandPaletteGoToPosition(
    const QString &jumpCommandKey, int64_t row, int64_t column,
    bool isPosition) {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  if (isPosition) {
    const int lineCount = editorController->getLineCount();
    const int64_t maxLine = std::max<int64_t>(1, lineCount);

    const int64_t clampedRow = std::clamp<int64_t>(row, 1, maxLine);
    const int64_t clampedCol = std::max<int64_t>(1, column);
    const auto adjustedRow = static_cast<std::int64_t>(clampedRow - 1);
    const auto adjustedColumn = static_cast<std::int64_t>(clampedCol - 1);

    neko::JumpCommandFfi jumpCommand{
        .kind = neko::JumpCommandKindFfi::ToPosition,
        .row = adjustedRow,
        .column = adjustedColumn,
        .line_target = neko::LineTargetFfi::Start,
        .document_target = neko::DocumentTargetFfi::Start,
    };

    appController->executeJumpCommand(jumpCommand);
  } else {
    appController->executeJumpKey(jumpCommandKey);
  }

  // TODO(scarlet): Turn these into signals?
  uiHandles.editorWidget->setFocus();
  uiHandles.editorWidget->onCursorChanged();
  uiHandles.gutterWidget->onCursorChanged();
  refreshStatusBarCursorInfo();
}

void WorkspaceCoordinator::commandPaletteCommand(const QString &key,
                                                 const QString &fullText) {
  rust::String rustKey;
  rust::String displayName;
  neko::CommandKindFfi kind;
  rust::String argument;

  const auto commands = AppController::getAvailableCommands();
  for (const auto &nekoCommand : commands) {
    if (key == QString::fromUtf8(nekoCommand.key)) {
      rustKey = nekoCommand.key;
      displayName = nekoCommand.display_name;
      kind = nekoCommand.kind;

      if (kind == neko::CommandKindFfi::JumpManagement) {
        argument = rust::String(fullText.toStdString());
      } else {
        argument = nekoCommand.argument;
      }

      break;
    }
  }

  if (rustKey.size() == 0) {
    qDebug() << "commandPaletteCommand: unknown key" << key
             << "fullText:" << fullText;
    return;
  }

  const auto beforeSnapshot = tabController->getTabsSnapshot();
  if (beforeSnapshot.active_present) {
    tabFlows.saveScrollOffsetsForActiveTab();
  }

  auto result = commandExecutor->execute(rustKey, displayName, kind, argument);

  for (auto &intent : result.intents) {
    switch (intent.kind) {
    case neko::UiIntentKindFfi::ToggleFileExplorer:
      fileExplorerToggled();
      break;
    case neko::UiIntentKindFfi::ApplyTheme:
      emit themeChanged(std::string(intent.argument_str.c_str()));
      break;
    case neko::UiIntentKindFfi::OpenConfig: {
      const int tabId = static_cast<int>(intent.argument_u64);
      const std::string &path = std::string(intent.argument_str);

      // If config was already open, just activate it
      for (const auto &tab : beforeSnapshot.tabs) {
        if (tab.path_present && tab.path == path) {
          tabController->setActiveTab(static_cast<int>(tab.id));
          return;
        }
      }

      tabController->notifyTabOpenedFromCore(tabId);
      break;
    }
    case neko::UiIntentKindFfi::ShowJumpAliases: {
      // const auto aliases = intent.jump_aliases;
      // TODO(scarlet): Show jump aliases
      break;
    }
    }
  }
}

void WorkspaceCoordinator::openFile() {
  // Check if there is an active tab - if so, use the parent path
  const auto snapshot = tabController->getTabsSnapshot();
  QString startingPath;

  if (snapshot.active_present) {
    const int activeId = static_cast<int>(snapshot.active_id);
    for (const auto &tab : snapshot.tabs) {
      if (tab.path_present && tab.id == activeId) {
        startingPath = QString::fromUtf8(tab.path);
        break;
      }
    }
  }

  QString initialDir;
  if (!startingPath.isEmpty()) {
    QFileInfo info(startingPath);
    initialDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  }

  const QString filePath =
      DialogService::openFileSelectionDialog(initialDir, uiHandles.window);
  if (filePath.isEmpty()) {
    return;
  }

  auto targetTabId = tabController->addTab();
  const auto result =
      appController->openFile(targetTabId, filePath.toStdString());

  if (result.success) {
    emit fileOpened(result.snapshot);
  }
}

void WorkspaceCoordinator::fileSelected(const std::string &path,
                                        bool focusEditor) {
  // Save scroll offsets if there is an active tab
  {
    const auto snapshot = tabController->getTabsSnapshot();
    if (snapshot.active_present) {
      tabFlows.saveScrollOffsetsForActiveTab();
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
  auto result = appController->openFile(path);

  if (!result.success) {
    tabController->closeTabs(neko::CloseTabOperationTypeFfi::Single, newTabId,
                             false);
    return;
  }

  tabController->fileOpened(result.snapshot);
}

void WorkspaceCoordinator::fileSaved(bool saveAs) {
  tabFlows.fileSaved(saveAs);
}

void WorkspaceCoordinator::openConfig() {
  const auto configPath = appConfigService->getConfigPath();

  if (!configPath.empty()) {
    fileSelected(configPath, true);
  }
}

void WorkspaceCoordinator::handleTabCommand(const std::string &commandId,
                                            const neko::TabContextFfi &ctx,
                                            bool forceClose) {
  tabFlows.handleTabCommand(commandId, ctx, forceClose);
}

void WorkspaceCoordinator::copyTabPath(int tabId) {
  tabFlows.copyTabPath(tabId);
}

void WorkspaceCoordinator::tabTogglePin(int tabId, bool tabIsPinned) {
  tabFlows.tabTogglePin(tabId, tabIsPinned);
}

void WorkspaceCoordinator::newTab() { tabFlows.newTab(); }

void WorkspaceCoordinator::tabUnpinned(int tabId) {
  tabFlows.tabUnpinned(tabId);
}

void WorkspaceCoordinator::bufferChanged() { tabFlows.bufferChanged(); }

void WorkspaceCoordinator::tabChanged(int tabId) { tabFlows.tabChanged(tabId); }

SaveResult WorkspaceCoordinator::saveTab(int tabId, bool isSaveAs) {
  return tabFlows.saveTab(tabId, isSaveAs);
}

void WorkspaceCoordinator::revealActiveTab() {
  const auto snapshot = tabController->getTabsSnapshot();
  if (!snapshot.active_present) {
    return;
  }

  neko::TabContextFfi ctx;
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

void WorkspaceCoordinator::moveTabBy(int delta, bool useHistory) {
  tabFlows.moveTabBy(delta, useHistory);
}

// TODO(scarlet): Wrap the neko:: type eventually so it doesn't leak into
// widgets
void WorkspaceCoordinator::closeTabs(
    neko::CloseTabOperationTypeFfi operationType, int anchorTabId,
    bool forceClose) {
  tabFlows.closeTabs(operationType, anchorTabId, forceClose);
}

void WorkspaceCoordinator::applyInitialState() {
  const auto snapshot = tabController->getTabsSnapshot();

  int index = 0;
  for (const auto &tab : snapshot.tabs) {
    TabPresentation presentation;
    presentation.id = static_cast<int>(tab.id);
    presentation.title = QString::fromUtf8(tab.title);
    presentation.path = QString::fromUtf8(tab.path);
    presentation.pinned = tab.pinned;
    presentation.modified = tab.modified;

    uiHandles.tabBarWidget->addTab(presentation, index++);
  }

  if (snapshot.active_present) {
    uiHandles.tabBarWidget->setCurrentTabId(
        static_cast<int>(snapshot.active_id));
  }

  refreshStatusBarCursorInfo();

  auto cfg = appConfigService->getSnapshot();
  if (!cfg.file_explorer.shown) {
    uiHandles.fileExplorerWidget->hide();
  }

  uiHandles.editorWidget->setFocus();
}

void WorkspaceCoordinator::refreshUiForActiveTab(bool focusEditor) {
  const auto snapshot = tabController->getTabsSnapshot();

  if (!snapshot.active_present) {
    uiHandles.tabBarContainerWidget->hide();
    uiHandles.editorWidget->hide();
    uiHandles.gutterWidget->hide();
    uiHandles.emptyStateWidget->show();
    uiHandles.fileExplorerWidget->setFocus();
    return;
  }

  // IMPORTANT: Set active editor before re-showing widgets, otherwise
  // Qt layout triggers a resize event (which queries the line count with the
  // old editor reference)
  auto &activeEditor = appController->getActiveEditorMut();
  setActiveEditor(&activeEditor);

  uiHandles.emptyStateWidget->hide();
  uiHandles.tabBarContainerWidget->show();
  uiHandles.editorWidget->show();
  uiHandles.gutterWidget->show();
  uiHandles.statusBarWidget->showCursorPositionInfo();

  uiHandles.editorWidget->updateDimensions();
  uiHandles.editorWidget->redraw();
  uiHandles.gutterWidget->updateDimensions();
  uiHandles.gutterWidget->redraw();

  tabFlows.restoreScrollOffsetsForActiveTab();
  refreshStatusBarCursorInfo();

  if (focusEditor) {
    uiHandles.editorWidget->setFocus();
  }
}

void WorkspaceCoordinator::setActiveEditor(neko::Editor *editor) {
  editorController->setEditor(editor);

  uiHandles.editorWidget->setEditorController(editorController);
  uiHandles.gutterWidget->setEditorController(editorController);
}

void WorkspaceCoordinator::refreshStatusBarCursorInfo() {
  if (editorController == nullptr) {
    return;
  }

  const auto cursorPosition = editorController->getLastAddedCursor();
  const int numberOfCursors =
      static_cast<int>(editorController->getCursorPositions().size());

  uiHandles.statusBarWidget->updateCursorPosition(
      cursorPosition.row, cursorPosition.column, numberOfCursors);
}
