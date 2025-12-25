#ifndef WORKSPACE_UI_H
#define WORKSPACE_UI_H

#include "close_decision.h"
#include <QList>
#include <QString>
#include <functional>

struct WorkspaceUi {
  std::function<QString(int tabId)> promptSaveAsPath;
  std::function<CloseDecision(const QList<int> &ids)> confirmCloseTabs;
};

#endif
