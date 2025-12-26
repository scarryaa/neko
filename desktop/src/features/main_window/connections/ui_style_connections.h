#ifndef UI_STYLE_CONNECTIONS_H
#define UI_STYLE_CONNECTIONS_H

class UiStyleManager;

#include "features/main_window/workspace_ui_handles.h"
#include <QObject>

class UiStyleConnections : public QObject {
  Q_OBJECT

public:
  struct UiStyleConnectionsProps {
    WorkspaceUiHandles uiHandles;
    UiStyleManager *uiStyleManager;
  };

  explicit UiStyleConnections(const UiStyleConnectionsProps &props,
                              QObject *parent = nullptr);
  ~UiStyleConnections() override = default;
};

#endif
