#include "features/main_window/controllers/ui_style_manager.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include "utils/ui_utils.h"

namespace k {
constexpr double commandPaletteFontSizeMultiplier = 1.5;
} // namespace k

UiStyleManager::UiStyleManager(const UiStyleManagerProps &props,
                               QObject *parent)
    : QObject(parent), appConfigService(props.appConfigService) {}

QFont UiStyleManager::interfaceFont() const { return m_interfaceFont; }

QFont UiStyleManager::fileExplorerFont() const { return m_fileExplorerFont; }

QFont UiStyleManager::editorFont() const { return m_editorFont; }

QFont UiStyleManager::commandPaletteFont() const {
  auto commandPaletteFont = m_interfaceFont;
  commandPaletteFont.setPointSizeF(m_interfaceFont.pointSizeF() *
                                   k::commandPaletteFontSizeMultiplier);
  return commandPaletteFont;
}

void UiStyleManager::handleConfigChanged(
    const neko::ConfigSnapshotFfi &configSnapshot) {
  auto interfaceFont =
      UiUtils::makeFont(QString::fromUtf8(configSnapshot.interface_font_family),
                        configSnapshot.interface_font_size);
  auto fileExplorerFont = UiUtils::makeFont(
      QString::fromUtf8(configSnapshot.file_explorer_font_family),
      configSnapshot.file_explorer_font_size);
  auto editorFont =
      UiUtils::makeFont(QString::fromUtf8(configSnapshot.editor_font_family),
                        configSnapshot.editor_font_size);

  if (interfaceFont != m_interfaceFont) {
    m_interfaceFont = interfaceFont;
    emit interfaceFontChanged(m_interfaceFont);
    emit commandPaletteFontChanged(commandPaletteFont());
  }

  if (fileExplorerFont != m_fileExplorerFont) {
    m_fileExplorerFont = fileExplorerFont;
    emit fileExplorerFontChanged(m_fileExplorerFont);
  }

  if (editorFont != m_editorFont) {
    m_editorFont = editorFont;
    emit editorFontChanged(m_editorFont);
  }
}
