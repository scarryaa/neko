#include "workspace_coordinator.h"

WorkspaceCoordinator::WorkspaceCoordinator(
    WorkspaceController *workspaceController, TabController *tabController,
    AppStateController *appStateController, EditorController *editorController,
    neko::ConfigManager *configManager, neko::ThemeManager *themeManager,
    WorkspaceUiHandles uiHandles, QObject *parent)
    : workspaceController(workspaceController), tabController(tabController),
      appStateController(appStateController),
      editorController(editorController), uiHandles(uiHandles),
      configManager(configManager), themeManager(themeManager),
      QObject(parent) {
  // TabController -> WorkspaceCoordinator
  connect(tabController, &TabController::tabListChanged, this,
          &WorkspaceCoordinator::updateTabBar);
  connect(tabController, &TabController::activeTabChanged, this,
          &WorkspaceCoordinator::refreshUiForActiveTab);

  // WorkspaceCoordinator -> TabBarWidget
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabCloseRequested, this,
          &WorkspaceCoordinator::closeTab);
  connect(uiHandles.tabBarWidget, &TabBarWidget::currentChanged, this,
          &WorkspaceCoordinator::tabChanged);
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabPinnedChanged, this,
          &WorkspaceCoordinator::tabChanged);
  connect(uiHandles.tabBarWidget, &TabBarWidget::tabUnpinRequested, this,
          &WorkspaceCoordinator::tabUnpinned);
  connect(uiHandles.tabBarWidget, &TabBarWidget::newTabRequested, this,
          &WorkspaceCoordinator::newTab);

  // WorkspaceCoordinator -> EditorWidget
  connect(uiHandles.editorWidget, &EditorWidget::newTabRequested, this,
          &WorkspaceCoordinator::newTab);

  neko::Editor &activeEditor = appStateController->getActiveEditorMut();
  setActiveEditor(&activeEditor);
}

WorkspaceCoordinator::~WorkspaceCoordinator() {}

void WorkspaceCoordinator::fileExplorerToggled() {
  bool isOpen = uiHandles.fileExplorerWidget->isHidden();

  if (isOpen) {
    uiHandles.fileExplorerWidget->show();
  } else {
    uiHandles.fileExplorerWidget->hide();
  }

  auto snapshot = configManager->get_config_snapshot();
  snapshot.file_explorer_shown = isOpen;
  configManager->apply_config_snapshot(snapshot);
}

void WorkspaceCoordinator::cursorPositionClicked() {
  if (tabController->getTabsEmpty()) {
    return;
  }

  neko::CursorPosition cursor = editorController->getLastAddedCursor();
  int lineCount = editorController->getLineCount();

  if (lineCount == 0) {
    return;
  }

  auto maxCol =
      std::max<size_t>(1, editorController->getLineLength(cursor.row));
  auto lastLineMaxCol =
      std::max<size_t>(1, editorController->getLineLength(lineCount - 1));

  uiHandles.commandPaletteWidget->jumpToRowColumn(
      cursor.row, cursor.col, maxCol, lineCount, lastLineMaxCol);
}

void WorkspaceCoordinator::commandPaletteGoToPosition(int row, int col) {
  if (tabController->getTabsEmpty()) {
    return;
  }

  editorController->moveTo(row, col, true);
  uiHandles.editorWidget->setFocus();
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

  auto commandFfi = neko::new_command(kind, std::move(argument));
  auto result =
      neko::execute_command(commandFfi, *configManager, *themeManager);

  for (auto &intent : result.intents) {
    switch (intent.kind) {
    case neko::UiIntentKindFfi::ToggleFileExplorer:
      fileExplorerToggled();
      emit onFileExplorerToggledViaShortcut(
          !uiHandles.fileExplorerWidget->isHidden());
      break;
    case neko::UiIntentKindFfi::ApplyTheme:
      emit themeChanged(std::string(intent.argument.c_str()));
      break;
    }
  }
}

void WorkspaceCoordinator::fileSelected(const std::string &path,
                                        bool focusEditor) {
  int index = tabController->getTabIndexByPath(path);
  int id = tabController->getTabId(index);

  if (index != -1) {
    // Save current scroll offset
    saveScrollOffsetsForActiveTab();

    tabController->setActiveTab(id);
    refreshUiForActiveTab(focusEditor);
    updateTabBar();
  }
}

void WorkspaceCoordinator::fileSaved(bool saveAs) {
  const int activeId = tabController->getActiveTabId();
  const bool success = workspaceController->saveTab(activeId, saveAs);

  if (success) {
    uiHandles.tabBarWidget->setTabModified(activeId, false);
    updateTabBar();
  }
}

void WorkspaceCoordinator::openConfig() {
  auto configPath = configManager->get_config_path();

  if (!configPath.empty()) {
    fileSelected(configPath.c_str(), true);
  }
}

void WorkspaceCoordinator::tabCopyPath(int id) {
  const QString path = tabController->getTabPath(id);

  if (path.isEmpty())
    return;

  QApplication::clipboard()->setText(path);
}

void WorkspaceCoordinator::tabReveal(int id) {
  const QString path = tabController->getTabPath(id);

  if (path.isEmpty())
    return;

  uiHandles.fileExplorerWidget->showItem(path);
}

void WorkspaceCoordinator::newTab() {
  saveScrollOffsetsForActiveTab();

  tabController->addTab();

  updateTabBar();
  refreshUiForActiveTab(true);
}

void WorkspaceCoordinator::tabChanged(int id) {
  saveScrollOffsetsForActiveTab();
  tabController->setActiveTab(id);

  refreshUiForActiveTab(true);
  updateTabBar();
}

void WorkspaceCoordinator::tabUnpinned(int id) {
  tabController->unpinTab(id);
  updateTabBar();
}

void WorkspaceCoordinator::bufferChanged() {
  int activeId = tabController->getActiveTabId();
  bool modified = tabController->getTabModified(activeId);

  uiHandles.tabBarWidget->setTabModified(activeId, modified);
}

CloseDecision
WorkspaceCoordinator::showTabCloseConfirmationDialog(const QList<int> &ids) {
  int modifiedCount = 0;

  for (int i = 0; i < ids.size(); i++) {
    if (tabController->getTabModified(ids[i])) {
      modifiedCount++;
    }
  }

  if (modifiedCount == 0) {
    return CloseDecision::DontSave;
  }

  QMessageBox box(QMessageBox::Warning, tr("Close Tabs"),
                  tr("%1 tab%2 unsaved edits.")
                      .arg(modifiedCount)
                      .arg(modifiedCount > 1 ? "s have" : " has"),
                  QMessageBox::NoButton, uiHandles.window);

  auto *saveBtn = box.addButton(tr("Save"), QMessageBox::AcceptRole);
  auto *dontSaveBtn =
      box.addButton(tr("Don't Save"), QMessageBox::DestructiveRole);
  auto *cancelBtn = box.addButton(QMessageBox::Cancel);

  box.setDefaultButton(cancelBtn);
  box.setEscapeButton(cancelBtn);

  box.exec();

  if (box.clickedButton() == saveBtn)
    return CloseDecision::Save;
  if (box.clickedButton() == dontSaveBtn)
    return CloseDecision::DontSave;

  return CloseDecision::Cancel;
}

SaveResult WorkspaceCoordinator::saveTab(int id, bool isSaveAs) {
  if (workspaceController->saveTab(id, isSaveAs)) {
    uiHandles.tabBarWidget->setTabModified(id, false);

    updateTabBar();

    return SaveResult::Saved;
  }

  qDebug() << "Save as failed";
  return SaveResult::Failed;
}

void WorkspaceCoordinator::closeTab(int id, bool forceClose) {
  // TODO: Focus remaining tabs after close process
  auto close = [this](int id, bool forceClose) {
    // Save current scroll offset before closing
    saveScrollOffsetsForActiveTab();

    auto ids = workspaceController->closeTab(id, forceClose);
    if (!ids.empty()) {
      handleTabsClosed(ids);
    }
  };

  if (tabController->getTabCount() == 0) {
    // Close the window
    QApplication::quit();
    return;
  }

  bool isPinned = tabController->getIsPinned(id);
  if (isPinned && !forceClose) {
    return;
  }

  close(id, forceClose && !isPinned);
}

void WorkspaceCoordinator::closeLeftTabs(int id, bool forceClose) {
  // TODO: Focus remaining tabs after close process
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeLeft(id, forceClose);
  if (!ids.empty()) {
    handleTabsClosed(ids);
  }
}

void WorkspaceCoordinator::closeRightTabs(int id, bool forceClose) {
  // TODO: Focus remaining tabs after close process
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeRight(id, forceClose);
  if (!ids.empty()) {
    handleTabsClosed(ids);
  }
}

void WorkspaceCoordinator::closeOtherTabs(int id, bool forceClose) {
  // TODO: Focus remaining tabs after close process
  saveScrollOffsetsForActiveTab();

  auto ids = workspaceController->closeOthers(id, forceClose);

  if (!ids.empty()) {
    handleTabsClosed(ids);
  }
}

void WorkspaceCoordinator::applyInitialState() {
  updateTabBar();
  refreshStatusBarCursorInfo();

  auto snapshot = configManager->get_config_snapshot();
  if (!snapshot.file_explorer_shown) {
    uiHandles.fileExplorerWidget->hide();
  }

  uiHandles.fileExplorerWidget->loadSavedDir();
  editorController->refresh();
  uiHandles.editorWidget->setFocus();
}

void WorkspaceCoordinator::saveScrollOffsetsForActiveTab() {
  if (tabController->getTabsEmpty())
    return;

  int id = tabController->getActiveTabId();

  double x = uiHandles.editorWidget->horizontalScrollBar()->value();
  double y = uiHandles.editorWidget->verticalScrollBar()->value();
  neko::ScrollOffsetFfi scrollOffsets = {static_cast<int32_t>(x),
                                         static_cast<int32_t>(y)};

  tabController->setTabScrollOffsets(id, scrollOffsets);
}

void WorkspaceCoordinator::restoreScrollOffsetsForActiveTab() {
  int id = tabController->getActiveTabId();
  auto offsets = tabController->getTabScrollOffsets(id);

  uiHandles.editorWidget->horizontalScrollBar()->setValue(offsets.x);
  uiHandles.editorWidget->verticalScrollBar()->setValue(offsets.y);
}

void WorkspaceCoordinator::refreshUiForActiveTab(bool focusEditor) {
  if (tabController->getTabsEmpty()) {
    uiHandles.tabBarContainerWidget->hide();
    uiHandles.editorWidget->hide();
    uiHandles.gutterWidget->hide();
    uiHandles.emptyStateWidget->show();
    uiHandles.fileExplorerWidget->setFocus();

    return;
  }

  uiHandles.emptyStateWidget->hide();
  uiHandles.tabBarContainerWidget->show();
  uiHandles.editorWidget->show();
  uiHandles.gutterWidget->show();
  uiHandles.statusBarWidget->showCursorPositionInfo();

  // Update active editor
  neko::Editor &activeEditor = appStateController->getActiveEditorMut();
  setActiveEditor(&activeEditor);

  uiHandles.editorWidget->updateDimensions();
  uiHandles.editorWidget->redraw();
  uiHandles.gutterWidget->updateDimensions();
  uiHandles.gutterWidget->redraw();

  restoreScrollOffsetsForActiveTab();
  refreshStatusBarCursorInfo();

  if (focusEditor)
    uiHandles.editorWidget->setFocus();
}

void WorkspaceCoordinator::updateTabBar() {
  auto rawTitles = tabController->getTabTitles();
  int count = rawTitles.size();

  QStringList tabTitles;
  QStringList tabPaths;
  for (int i = 0; i < rawTitles.size(); i++) {
    tabTitles.append(QString::fromUtf8(rawTitles[i]));

    const int id = tabController->getTabId(i);
    auto path = tabController->getTabPath(id);
    tabPaths.append(path);
  }

  rust::Vec<bool> modifieds = tabController->getTabModifiedStates();
  rust::Vec<bool> pinnedStates = tabController->getTabPinnedStates();

  uiHandles.tabBarWidget->setTabs(tabTitles, tabPaths, modifieds, pinnedStates);
  uiHandles.tabBarWidget->setCurrentId(tabController->getActiveTabId());

  if (rawTitles.size() != 0) {
    setActiveEditor(&appStateController->getActiveEditorMut());
  } else {
    setActiveEditor(nullptr);
  }
}

void WorkspaceCoordinator::handleTabsClosed(const QList<int> &ids) {
  int newTabCount = tabController->getTabCount();
  uiHandles.statusBarWidget->onTabClosed(newTabCount);
  refreshUiForActiveTab(true);
  updateTabBar();
}

void WorkspaceCoordinator::setActiveEditor(neko::Editor *editor) {
  editorController->setEditor(editor);

  uiHandles.editorWidget->setEditorController(editorController);
  uiHandles.gutterWidget->setEditorController(editorController);
}

void WorkspaceCoordinator::refreshStatusBarCursorInfo() {
  if (!editorController) {
    return;
  }

  auto cursorPosition = editorController->getLastAddedCursor();
  int numberOfCursors = editorController->getCursorPositions().size();
  uiHandles.statusBarWidget->updateCursorPosition(
      cursorPosition.row, cursorPosition.col, numberOfCursors);
}
