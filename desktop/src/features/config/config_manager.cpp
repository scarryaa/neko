#include "config_manager.h"
#include "features/config/config.h"

ConfigManager::ConfigManager() : config(Config()) { loadConfig(); }

ConfigManager::~ConfigManager() { saveConfig(); }

void ConfigManager::loadConfig() {
  QString configDir;
#if defined(Q_OS_MACOS) || defined(Q_OS_MAC) || defined(Q_OS_LINUX)
  configDir = QDir::homePath() + "/.config/neko/";
#else
  configDir = QDir::homePath() + "/config/neko/";
#endif

  configPath = configDir.toStdString() + CONFIG_FILE_NAME;

  // Ensure the directory exists
  QDir dir(configDir);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QFile configFile(QString::fromStdString(configPath));

  if (!configFile.exists()) {
    // Create the file if it doesn't exist
    if (configFile.open(QIODevice::WriteOnly)) {
      configFile.write(Config().toJson());
      configFile.close();
    }
  } else {
    // Load existing config
    if (configFile.open(QIODevice::ReadOnly)) {
      QByteArray data = configFile.readAll();
      config.parse(data);
      configFile.close();
    }
  }
}

void ConfigManager::saveConfig() {
  QString configDir =
      QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
  configPath = configDir.toStdString() + "/" + CONFIG_FILE_NAME;

  // Ensure the directory exists
  QDir dir(configDir);
  if (!dir.exists()) {
    dir.mkpath(".");
  }

  QFile configFile(QString::fromStdString(configPath));

  if (configFile.open(QIODevice::WriteOnly)) {
    configFile.write(config.toJson());
    configFile.close();
  }
}
