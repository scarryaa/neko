#include "file_io_service.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QList>
#include <QMimeData>
#include <QUrl>

namespace k {
// Custom MIME type to identify cut operations.
static const char *cutMimeType = "application/x-neko-cut";
static const char *duplicateSuffix = " copy";
} // namespace k

// Cuts a file or directory (and its contents), placing it on the clipboard and
// marking it as a cut operation.
void FileIoService::cut(const QString &itemPath) {
  auto *mimeData = new QMimeData();
  QList<QUrl> urls;

  urls.append(QUrl::fromLocalFile(itemPath));
  mimeData->setUrls(urls);
  // Mark this clipboard data as a cut operation.
  mimeData->setData(k::cutMimeType, "1");

  QApplication::clipboard()->setMimeData(mimeData);
}

// Copies a file or directory (and its contents) to the clipboard.
void FileIoService::copy(const QString &itemPath) {
  auto *mimeData = new QMimeData();
  QList<QUrl> urls;

  urls.append(QUrl::fromLocalFile(itemPath));
  mimeData->setUrls(urls);

  QApplication::clipboard()->setMimeData(mimeData);
}

// Pastes an item from the clipboard.
//
// Returns `true` on success and `false` on failure.
// TODO(scarlet): Break this down.
// TODO(scarlet): Check if the provided directory path is actually a
// directory; either return if not or adjust it to the parent directory.
FileIoService::PasteResult
FileIoService::paste(const QString &targetDirectory) {
  // Get the data from the clipboard.
  const QMimeData *mimeData = QApplication::clipboard()->mimeData();
  PasteResult result{.success = false,
                     .wasCutOperation = false,
                     .items = QVector<PasteItem>()};

  // If there are no urls (i.e. file/directory paths), nothing to do.
  if (!mimeData->hasUrls()) {
    return result;
  }

  const bool isCutOperation = mimeData->hasFormat(k::cutMimeType);
  QList<QUrl> urls = mimeData->urls();

  for (const auto &url : urls) {
    const QString srcPath = url.toLocalFile();
    QFileInfo srcInfo(srcPath);

    if (!srcInfo.exists()) {
      qDebug() << "Source does not exist:" << srcPath;
      continue;
    }

    const QString destPath =
        targetDirectory + QDir::separator() + srcInfo.fileName();

    // Avoid moving/copying a directory into itself or its own subtree.
    if (destPath == srcPath && srcInfo.isDir()) {
      qDebug() << "Cannot copy a directory into itself:" << destPath;
      continue;
    }

    // If the source is a directory, copy its contents.
    if (srcInfo.isDir()) {
      if (isCutOperation) {
        // It's a cut operation; try to move (rename) the directory.
        if (!QDir().rename(srcPath, destPath)) {
          qDebug() << "Rename (cut) failed:" << srcPath;
        }
      } else {
        // It's a copy operation.
        if (!copyRecursively(srcPath, destPath)) {
          qDebug() << "Failed to copy directory:" << srcPath;
        }
      }
    } else {
      // Source is a file.
      if (QFile::exists(destPath)) {
        // TODO(scarlet): Show a confirmation dialog to cancel, replace the
        // file, or keep both. For now, just skip.
        qDebug() << "File already exists:" << destPath;
        continue;
      }

      if (isCutOperation) {
        // It's a cut operation on a single file.
        if (!QFile::rename(srcPath, destPath)) {
          qDebug() << "Rename (cut) failed:" << srcPath;
        }
      } else {
        // Regular copy operation on a single file.
        if (!QFile::copy(srcPath, destPath)) {
          qDebug() << "Failed to copy file:" << srcPath;
        }
      }
    }

    PasteItem item{.originalPath = srcPath, .newPath = destPath};
    result.items.push_back(item);
  }

  result.success = true;
  result.wasCutOperation = isCutOperation;

  return result;
}

// Attempts to duplicate the provided file or directory (and its contents),
// without placing it on the clipboard.
FileIoService::DuplicateResult
FileIoService::duplicate(const QString &itemPath) {
  QFileInfo info(itemPath);
  DuplicateResult failureResult = {.success = false, .newPath = ""};

  if (!info.exists()) {
    qDebug() << "Provided path does not exist:" << itemPath;
    return failureResult;
  }

  const QString dirPath = info.dir().absolutePath();
  const QString baseName = info.completeBaseName();
  const QString suffix = info.completeSuffix();
  const QString extension = suffix.isEmpty() ? "" : "." + suffix;
  const QString duplicateSuffix = QString(k::duplicateSuffix);
  QString baseDuplicateName = baseName + duplicateSuffix;
  QString candidateName = baseDuplicateName + extension;
  QString destPath = dirPath + QDir::separator() + candidateName;

  // Append " copy" to the destination file/directory name until it does not
  // overlap with existing items.
  while (QFile::exists(destPath)) {
    baseDuplicateName += duplicateSuffix;
    candidateName = baseDuplicateName + extension;
    destPath = dirPath + QDir::separator() + candidateName;
  }

  if (info.isDir()) {
    // Provided path is a directory.
    if (!copyRecursively(itemPath, destPath)) {
      qDebug() << "Failed to duplicate directory:" << itemPath;
      return failureResult;
    }
  } else {
    // Provided path is a file.
    if (!QFile::copy(itemPath, destPath)) {
      qDebug() << "Failed to duplicate file:" << itemPath;
      return failureResult;
    }
  }

  return {.success = true, .newPath = destPath};
}

// Attempts to delete the provided file or directory (and its contents).
//
// Returns `true` on success and `false` on failure.
bool FileIoService::deleteItem(const QString &itemPath) {
  QFileInfo info(itemPath);

  if (!info.exists()) {
    qDebug() << "Provided path does not exist:" << itemPath;
    return false;
  }

  // Figure out the parent directory.
  const QString parentPath = info.dir().absolutePath();

  if (info.isDir()) {
    // Provided path is a directory.
    QDir dir(itemPath);

    if (!dir.removeRecursively()) {
      qDebug() << "Failed to remove directory:" << itemPath;
      return false;
    }
  } else {
    // Provided path is a file.
    QFile file(itemPath);

    if (!file.remove()) {
      qDebug() << "Failed to remove file:" << itemPath;
      return false;
    }
  }

  return true;
}

bool FileIoService::copyRecursively(const QString &sourceFolder,
                                    const QString &destFolder) {
  QDir sourceDir(sourceFolder);
  QDir destDir(destFolder);

  if (!sourceDir.exists()) {
    return false;
  }

  // Make the destination directory if it does not exist.
  if (!destDir.exists()) {
    destDir.mkpath(".");
  }

  // Get the list of files in the source directory.
  QStringList files = sourceDir.entryList(QDir::Files | QDir::Dirs |
                                          QDir::NoDotAndDotDot | QDir::Hidden);

  for (const QString &file : files) {
    QString srcName = sourceFolder + QDir::separator() + file;
    QString destName = destFolder + QDir::separator() + file;
    QFileInfo info(srcName);

    if (info.isDir()) {
      copyRecursively(srcName, destName);
    } else {
      if (!QFile::copy(srcName, destName)) {
        qDebug() << "Failed to copy file:" << srcName;
      }
    }
  }

  return true;
}
