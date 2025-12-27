#ifndef WORKSPACE_UI_H
#define WORKSPACE_UI_H

#include "close_decision.h"
#include <QList>
#include <QString>
#include <functional>

struct WorkspaceUi {
  std::function<QString(std::optional<QString> initialDirectory,
                        std::optional<QString> initialFileName)>
      promptSaveAsPath;
  std::function<void(int tabId)> focusTab;
  std::function<CloseDecision(const QList<int> &ids)> confirmCloseTabs;
  std::function<QString()> promptFileExplorerDirectory;
  std::function<QString(std::optional<QString> initialDirectory)> openFile;
};

#endif
