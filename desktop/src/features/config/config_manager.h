#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include "features/config/config.h"
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QString>
#include <string>

class ConfigManager {
public:
  ConfigManager(const ConfigManager &) = delete;
  ConfigManager &operator=(const ConfigManager &) = delete;

  static ConfigManager &getInstance() {
    static ConfigManager instance;
    return instance;
  }

  void loadConfig();
  void saveConfig();
  Config &getConfig() { return config; }

private:
  ConfigManager();
  ~ConfigManager();

  Config config;
  std::string configPath;

  std::string CONFIG_FILE_NAME = "neko_config.json";
};

#endif // CONFIG_MANAGER_H
