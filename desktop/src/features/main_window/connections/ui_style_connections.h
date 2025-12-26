#ifndef UI_STYLE_CONNECTIONS_H
#define UI_STYLE_CONNECTIONS_H

class UiStyleManager;
class AppConfigService;

#include "features/main_window/ui_handles.h"
#include <QObject>

class UiStyleConnections : public QObject {
  Q_OBJECT

public:
  struct UiStyleConnectionsProps {
    UiHandles uiHandles;
    UiStyleManager *uiStyleManager;
    AppConfigService *appConfigService;
  };

  explicit UiStyleConnections(const UiStyleConnectionsProps &props,
                              QObject *parent = nullptr);
  ~UiStyleConnections() override = default;
};

#endif
