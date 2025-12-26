#ifndef APP_CONFIG_SERVICE
#define APP_CONFIG_SERVICE

#include "types/ffi_types_fwd.h"
#include <QObject>

/// \class AppConfigService
/// \brief Qt-side wrapper over Rust `neko::ConfigManager`.
///
/// AppConfigService provides a Qt-side interface to the application
/// configuration managed in Rust. It wraps Rust methods rather than exposing
/// the FFI layer directly.
///
/// \section responsibilities Responsibilities
/// - Acts as the mutator for configuration in the UI layer.
/// - Emits signals when configuration values change.
/// - Provides accessors for reading config state.
///
/// \section separation Separation of Concerns
/// - This class operates at the data/model level (font sizes, directories,
/// flags, etc.) only.
/// - It does not construct UI objects or apply styling; converting config
/// values to `QFont`, colors, spacing, etc. is the responsibility of
/// `UiStyleManager` and other presentation-level components.
class AppConfigService : public QObject {
  Q_OBJECT
public:
  struct AppConfigServiceProps {
    neko::ConfigManager *configManager;
  };

  explicit AppConfigService(const AppConfigServiceProps &props,
                            QObject *parent = nullptr);

  [[nodiscard]] neko::ConfigSnapshotFfi getSnapshot() const;
  [[nodiscard]] std::string getConfigPath() const;

  // NOLINTNEXTLINE(readability-redundant-access-specifiers)
public slots:
  // TODO(scarlet): Consolidate these into a single 'update' method if possible?
  void setInterfaceFontSize(int fontSize);
  void setEditorFontSize(int fontSize);
  void setFileExplorerFontSize(int fontSize);
  void setFileExplorerDirectory(const std::string &path);
  void setFileExplorerShown(bool shown);
  void setFileExplorerWidth(double width);

  // For external mutations (FFI commands)
  void notifyExternalConfigChange();

signals:
  // NOTE: Currently only emitted for fonts
  void configChanged(const neko::ConfigSnapshotFfi &snapshot);
  void interfaceFontConfigChanged(QString fontFamily, int fontSize);
  void editorFontConfigChanged(QString fontFamily, int fontSize);
  void fileExplorerFontConfigChanged(QString fontFamily, int fontSize);

private:
  neko::ConfigManager *configManager;
};

#endif
