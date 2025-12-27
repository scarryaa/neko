#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

// Need to include actual headers due to rust::String
#include <neko-core/src/ffi/bridge.rs.h>

class AppConfigService;

struct CommandExecutorProps {
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  neko::AppState *appState;
  AppConfigService *appConfigService;
};

class CommandExecutor {
public:
  explicit CommandExecutor(const CommandExecutorProps &props);

  neko::CommandResultFfi execute(neko::CommandKindFfi kind,
                                 rust::String argument = rust::String(""));

private:
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  neko::AppState *appState;
  AppConfigService *appConfigService;
};

#endif // COMMAND_EXECUTOR_H
