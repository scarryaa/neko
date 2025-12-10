#include "config.h"

Config::Config(double editorFontSize, double fileExplorerFontSize)
    : editorFontSize(editorFontSize),
      fileExplorerFontSize(fileExplorerFontSize) {}

Config::Config() : editorFontSize(15.0), fileExplorerFontSize(15.0) {}

void Config::parse(QByteArray qb) {
  QJsonDocument doc = QJsonDocument::fromJson(qb);

  if (doc.isNull() || !doc.isObject()) {
    return;
  }

  QJsonObject json = doc.object();

  if (json.contains("editorFontSize") && json["editorFontSize"].isDouble()) {
    editorFontSize = json["editorFontSize"].toDouble();
  }

  if (json.contains("fileExplorerFontSize") &&
      json["fileExplorerFontSize"].isDouble()) {
    fileExplorerFontSize = json["fileExplorerFontSize"].toDouble();
  }
}

QByteArray Config::toJson() {
  QJsonObject json;

  json["editorFontSize"] = editorFontSize;
  json["fileExplorerFontSize"] = fileExplorerFontSize;

  QJsonDocument doc(json);
  return doc.toJson();
}
