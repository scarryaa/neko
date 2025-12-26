#include "command_executor.h"
#include "features/main_window/controllers/app_config_service.h"
#include "neko-core/src/ffi/bridge.rs.h"

CommandExecutor::CommandExecutor(const CommandExecutorProps &props)
    : configManager(props.configManager), themeManager(props.themeManager),
      appConfigService(props.appConfigService) {}

neko::CommandResultFfi CommandExecutor::execute(neko::CommandKindFfi kind,
                                                rust::String argument) {
  auto commandFfi = neko::new_command(kind, std::move(argument));
  auto result =
      neko::execute_command(commandFfi, *configManager, *themeManager);

  if (appConfigService != nullptr) {
    appConfigService->notifyExternalConfigChange();
  }

  return result;
}
