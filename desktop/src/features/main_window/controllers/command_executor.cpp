#include "command_executor.h"
#include "core/bridge/app_bridge.h"
#include "features/main_window/services/app_config_service.h"
#include "neko-core/src/ffi/bridge.rs.h"

CommandExecutor::CommandExecutor(const CommandExecutorProps &props)
    : configManager(props.configManager), themeManager(props.themeManager),
      appBridge(props.appBridge), appConfigService(props.appConfigService) {}

neko::CommandResultFfi CommandExecutor::execute(const rust::String &key,
                                                const rust::String &displayName,
                                                neko::CommandKindFfi kind,
                                                const rust::String &argument) {
  auto commandFfi = appBridge->getCommandController()->new_command(
      key, displayName, kind, argument);
  auto result = appBridge->getCommandController()->execute_command(
      commandFfi, *configManager, *themeManager);

  if (appConfigService != nullptr) {
    appConfigService->notifyExternalConfigChange();
  }

  return result;
}
