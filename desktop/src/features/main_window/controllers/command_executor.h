#ifndef COMMAND_EXECUTOR_H
#define COMMAND_EXECUTOR_H

// Need to include actual headers due to rust::String
#include <neko-core/src/ffi/bridge.rs.h>

class AppBridge;
class AppConfigService;

struct CommandExecutorProps {
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  AppBridge *appBridge;
  AppConfigService *appConfigService;
};

class CommandExecutor {
public:
  explicit CommandExecutor(const CommandExecutorProps &props);

  neko::CommandResultFfi execute(const rust::String &key,
                                 const rust::String &displayName,
                                 neko::CommandKindFfi kind,
                                 const rust::String &argument);

private:
  neko::ConfigManager *configManager;
  neko::ThemeManager *themeManager;
  AppBridge *appBridge;
  AppConfigService *appConfigService;
};

#endif // COMMAND_EXECUTOR_H
