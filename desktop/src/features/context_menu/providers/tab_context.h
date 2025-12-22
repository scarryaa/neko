#ifndef TAB_CONTEXT_H
#define TAB_CONTEXT_H

#include <QMetaType>
#include <QString>

struct TabContext {
  int tabIndex;
  int tabId;
  bool isPinned;
  bool isModified;
  bool canCloseOthers;
  QString filePath;
};

#endif
