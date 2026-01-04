#ifndef FILE_IO_SERVICE
#define FILE_IO_SERVICE

#include <QString>

class FileIoService {
public:
  struct DuplicateResult {
    bool success;
    QString newPath;
  };

  static void cut(const QString &itemPath);
  static void copy(const QString &itemPath);
  static bool paste(const QString &targetDirectory);
  static DuplicateResult duplicate(const QString &itemPath);
  static bool deleteItem(const QString &itemPath);

private:
  static bool copyRecursively(const QString &sourceFolder,
                              const QString &destFolder);
};

#endif
