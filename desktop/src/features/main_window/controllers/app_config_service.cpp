#include "app_config_service.h"
#include "neko-core/src/ffi/bridge.rs.h"

AppConfigService::AppConfigService(const AppConfigServiceProps &props,
                                   QObject *parent)
    : QObject(parent), configManager(props.configManager) {}

neko::ConfigSnapshotFfi AppConfigService::getSnapshot() const {
  return configManager->get_config_snapshot();
}

void AppConfigService::setInterfaceFontSize(int fontSize) {
  auto snapshot = configManager->get_config_snapshot();
  snapshot.interface_font_size = fontSize;
  configManager->apply_config_snapshot(snapshot);

  auto updatedSnapshot = configManager->get_config_snapshot();
  emit configChanged(updatedSnapshot);
}

void AppConfigService::setEditorFontSize(int fontSize) {
  auto snapshot = configManager->get_config_snapshot();
  snapshot.editor_font_size = fontSize;
  configManager->apply_config_snapshot(snapshot);

  auto updatedSnapshot = configManager->get_config_snapshot();
  emit configChanged(updatedSnapshot);
}

void AppConfigService::setFileExplorerFontSize(int fontSize) {
  auto snapshot = configManager->get_config_snapshot();
  snapshot.file_explorer_font_size = fontSize;
  configManager->apply_config_snapshot(snapshot);

  auto updatedSnapshot = configManager->get_config_snapshot();
  emit configChanged(updatedSnapshot);
}

void AppConfigService::setFileExplorerDirectory(const std::string &path) {
  auto snapshot = configManager->get_config_snapshot();
  snapshot.file_explorer_directory = path;
  configManager->apply_config_snapshot(snapshot);

  auto updatedSnapshot = configManager->get_config_snapshot();
  emit configChanged(updatedSnapshot);
}
