#ifndef UI_STYLE_MANAGER_H
#define UI_STYLE_MANAGER_H

class AppConfigService;
class ThemeProvider;

#include "types/ffi_types_fwd.h"
#include <QFont>
#include <QObject>

/// \class UiStyleManager
/// \brief Distributes UI style derived from application configuration.
///
/// UiStyleManager listens to configuration changes from `AppConfigService` and
/// converts the model-level values into UI representations (e.g. `QFont`). It
/// centralizes style derivation so that widgets recieve consistent,
/// application-wide visual styles.
///
/// \section responsibilities Responsibilities
/// - Subscribes to config change signals from `AppConfigService`.
/// - Converts config values (font family/size, etc.) into `QFont` objects.
/// - Emits UI style signals so widgets can react without directly accessing the
/// underlying configuration.
///
/// \section separation Separation of Concerns
/// - This class operates at the presentation level.
/// - It does not persist configuration or mutate config state; see
/// `AppConfigService`.
/// - Widgets depend on UiStyleManager for style objects.
class UiStyleManager : public QObject {
  Q_OBJECT
public:
  struct UiStyleManagerProps {
    AppConfigService *appConfigService;
    ThemeProvider *themeProvider;
  };

  struct FontSnapshot {
    QFont interfaceFont;
    QFont editorFont;
    QFont fileExplorerFont;
    QFont commandPaletteFont;
  };

  [[nodiscard]] FontSnapshot getCurrentFonts() const;

  explicit UiStyleManager(const UiStyleManagerProps &props,
                          QObject *parent = nullptr);

  [[nodiscard]] QFont interfaceFont() const;
  [[nodiscard]] QFont fileExplorerFont() const;
  [[nodiscard]] QFont editorFont() const;
  [[nodiscard]] QFont commandPaletteFont() const;

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  void onEditorFontSizeChangedByUser(double newFontSize);

signals:
  // TODO(scarlet): Consolidate these into a single 'fontChanged/configChanged'
  // signal
  void interfaceFontChanged(const QFont &newFont);
  void fileExplorerFontChanged(const QFont &newFont);
  void editorFontChanged(const QFont &newFont);
  void commandPaletteFontChanged(const QFont &newFont);

public slots:
  void handleConfigChanged(const neko::ConfigSnapshotFfi &configSnapshot);

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
private:
  AppConfigService *appConfigService;
  QFont m_interfaceFont;
  QFont m_fileExplorerFont;
  QFont m_editorFont;
};

#endif
