#include "app_config_service.h"
#include "neko-core/src/ffi/bridge.rs.h"

AppConfigService::AppConfigService(const AppConfigServiceProps &props,
                                   QObject *parent)
    : QObject(parent), configManager(props.configManager) {}

neko::ConfigSnapshotFfi AppConfigService::getSnapshot() const {
  return configManager->get_config_snapshot();
}

std::string AppConfigService::getConfigPath() const {
  return configManager->get_config_path().c_str();
}

void AppConfigService::updateConfig(
    const std::function<void(neko::ConfigSnapshotFfi &)> &mutator,
    EmitConfigChanged emitMode) {
  auto snapshot = configManager->get_config_snapshot();
  mutator(snapshot);
  configManager->apply_config_snapshot(snapshot);

  if (emitMode == EmitConfigChanged::Yes) {
    auto updatedSnapshot = configManager->get_config_snapshot();
    emit configChanged(updatedSnapshot);
  }
}

void AppConfigService::setInterfaceFontSize(int fontSize) {
  updateConfig(
      [fontSize](neko::ConfigSnapshotFfi &snapshot) {
        snapshot.interface_font_size = fontSize;
      },
      EmitConfigChanged::Yes);
}

void AppConfigService::setEditorFontSize(int fontSize) {
  updateConfig(
      [fontSize](neko::ConfigSnapshotFfi &snapshot) {
        snapshot.editor_font_size = fontSize;
      },
      EmitConfigChanged::Yes);
}

void AppConfigService::setFileExplorerFontSize(int fontSize) {
  updateConfig(
      [fontSize](neko::ConfigSnapshotFfi &snapshot) {
        snapshot.file_explorer_font_size = fontSize;
      },
      EmitConfigChanged::Yes);
}

void AppConfigService::setFileExplorerDirectory(const std::string &path) {
  updateConfig(
      [&path](neko::ConfigSnapshotFfi &snapshot) {
        snapshot.file_explorer_directory_present = true;
        snapshot.file_explorer_directory = path;
      },
      EmitConfigChanged::Yes);
}

void AppConfigService::setFileExplorerShown(bool shown) {
  updateConfig(
      [shown](neko::ConfigSnapshotFfi &snapshot) {
        snapshot.file_explorer_shown = shown;
      },
      EmitConfigChanged::No);
}

void AppConfigService::setFileExplorerWidth(double width) {
  updateConfig(
      [width](neko::ConfigSnapshotFfi &snapshot) {
        snapshot.file_explorer_width = static_cast<uint32_t>(width);
      },
      EmitConfigChanged::No);
}

void AppConfigService::notifyExternalConfigChange() {
  auto snapshot = configManager->get_config_snapshot();
  emit configChanged(snapshot);
}
