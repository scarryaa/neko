#ifndef FILE_IO_SERVICE
#define FILE_IO_SERVICE

#include <QFileInfo>
#include <QString>
#include <QVector>

class FileIoService {
public:
  struct DuplicateResult {
    bool success;
    QString newPath;
  };

  struct PasteItem {
    QString originalPath;
    QString newPath;
    bool originalWasDeleted = false;
  };

  struct PasteResult {
    bool success;
    bool wasCutOperation;
    QVector<PasteItem> items;
    bool originalWasDeleted = false;
  };

  static void cut(const QString &itemPath);
  static void copy(const QString &itemPath);
  static PasteResult paste(const QString &targetDirectory);
  static DuplicateResult duplicate(const QString &itemPath);
  static bool deleteItem(const QString &itemPath);

private:
  static bool copyRecursively(const QString &sourceFolder,
                              const QString &destFolder);
  static QString adjustDestinationPath(const QString &targetDirectory,
                                       const QFileInfo &srcInfo,
                                       const QString &itemFileName);
  static PasteItem handleDirectoryPaste(const QString &srcPath,
                                        const QString &destPath,
                                        bool isCutOperation);
  static PasteItem handleFilePaste(const QString &srcPath,
                                   const QString &destPath,
                                   bool isCutOperation);

  static void removeFromClipboard(const QString &pathToRemove);
};

#endif
