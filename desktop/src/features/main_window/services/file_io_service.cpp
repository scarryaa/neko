#include "file_io_service.h"
#include <QApplication>
#include <QClipboard>
#include <QDir>
#include <QList>
#include <QMimeData>
#include <QUrl>

// TODO(scarlet): Make enum for duplicate/paste/cut/etc cases?
namespace k {
// Custom MIME type to identify cut operations.
static const char *cutMimeType = "application/x-neko-cut";
// Suffix used for duplicate operations - e.g. "file1" becomes "file1 copy",
// "file1 copy" becomes "file1 copy copy", and so on.
static const char *duplicateSuffix = " copy";
} // namespace k

void FileIoService::removeFromClipboard(const QString &pathToRemove) {
  QClipboard *clipboard = QApplication::clipboard();
  const QMimeData *currentData = clipboard->mimeData();

  // If there is data on the clipboard, continue.
  if ((currentData != nullptr) && currentData->hasUrls()) {
    QList<QUrl> urls = currentData->urls();
    QList<QUrl> remainingUrls;
    QUrl urlToRemove = QUrl::fromLocalFile(pathToRemove);

    // Filter out the specific file.
    for (const QUrl &url : urls) {
      if (url != urlToRemove) {
        remainingUrls.append(url);
      }
    }

    if (remainingUrls.isEmpty()) {
      // If that was the only file, clear the clipboard completely.
      clipboard->clear();
    } else {
      // Otherwise, update clipboard with the remaining files.
      auto *newData = new QMimeData();

      newData->setUrls(remainingUrls);
      clipboard->setMimeData(newData);
    }
  }
}

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

    QString destPath =
        adjustDestinationPath(targetDirectory, srcInfo, url.fileName());
    PasteItem item;

    if (srcInfo.isDir()) {
      // If the source is a directory, copy its contents.
      item = handleDirectoryPaste(srcPath, destPath, isCutOperation);
    } else {
      // Source is a file.
      item = handleFilePaste(srcPath, destPath, isCutOperation);
    }

    result.items.push_back(item);
  }

  result.success = true;
  result.wasCutOperation = isCutOperation;

  return result;
}

// Adjusts the destination path an operation.
//
// If the provided path is a directory, it is adjusted to target within the
// directory.
//
// If the provided path is a file, it is adjusted to target the parent
// directory.
QString FileIoService::adjustDestinationPath(const QString &targetDirectory,
                                             const QFileInfo &srcInfo,
                                             const QString &itemFileName) {
  QFileInfo targetInfo(targetDirectory);
  QString destPath;

  // Determine if a file path or directory path was provided.
  if (targetInfo.isDir()) {
    // If the source path and the target path is the same, don't adjust so it
    // gets identified as a duplicate operation.
    if (srcInfo.path() + QDir::separator() + itemFileName == targetDirectory) {
      destPath = targetDirectory;
    } else {
      // Otherwise, adjust the path to target within the directory.
      destPath = targetDirectory + QDir::separator() + srcInfo.fileName();
    }
  } else {
    // A file path was provided; adjust the path to target the parent
    // directory.
    const QString parentPath = targetInfo.dir().path();
    destPath = parentPath + QDir::separator() + srcInfo.fileName();
  }

  return destPath;
}

// Helper to handle a directory paste operation.
//
// Handles cases like colliding paths (treated as a duplicate operation), a
// cut/paste sequence, or a normal copy/paste sequence.
FileIoService::PasteItem FileIoService::handleDirectoryPaste(
    const QString &srcPath, const QString &destPath, bool isCutOperation) {
  PasteItem item{.originalPath = srcPath, .newPath = destPath};

  if (isCutOperation) {
    // It's a cut operation; try to move (rename) the directory.
    if (!QFile::rename(srcPath, destPath)) {
      qDebug() << "Rename (cut) failed:" << srcPath
               << "trying duplicate instead.";

      // If the cut operation fails due to collision, duplicate it and delete
      // the original directory instead.
      auto duplicateResult = duplicate(srcPath);

      if (!duplicateResult.success) {
        // If the duplicate fails too, just return.
        qDebug() << "Duplicate (cut) failed:" << srcPath;

        return {.originalPath = srcPath, .newPath = srcPath};
      }

      // If it succeeded, delete the original directory.
      deleteItem(srcPath);

      // Remove the original directory from the clipboard.
      removeFromClipboard(srcPath);

      return {.originalPath = srcPath, .newPath = duplicateResult.newPath};
    }
  }

  // If the source and destination are the same, treat it as a duplicate
  // operation.
  if (destPath == srcPath) {
    const auto duplicateResult = duplicate(srcPath);

    // Convert the `DuplicateResult` to a `PasteItem`.
    item = {.originalPath = srcPath, .newPath = duplicateResult.newPath};
    return item;
  }

  // It's a copy operation.
  if (!copyRecursively(srcPath, destPath)) {
    qDebug() << "Failed to copy directory:" << srcPath;
  }

  return item;
}

// Helper to handle a file paste operation.
//
// Handles cases like colliding paths (treated as a duplicate operation), a
// cut/paste sequence, or a normal copy/paste sequence.
FileIoService::PasteItem FileIoService::handleFilePaste(const QString &srcPath,
                                                        const QString &destPath,
                                                        bool isCutOperation) {
  PasteItem item{.originalPath = srcPath, .newPath = destPath};

  if (isCutOperation) {
    // It's a cut operation on a single file.
    if (!QFile::rename(srcPath, destPath)) {
      qDebug() << "Rename (cut) failed:" << srcPath
               << "trying duplicate instead.";

      // If the cut operation fails due to collision, duplicate it and delete
      // the original file instead.
      auto duplicateResult = duplicate(srcPath);

      if (!duplicateResult.success) {
        // If the duplicate fails too, just return.
        qDebug() << "Duplicate (cut) failed:" << srcPath;

        return {.originalPath = srcPath, .newPath = srcPath};
      }

      // If it succeeded, delete the original file.
      deleteItem(srcPath);

      // Remove the original file from the clipboard.
      removeFromClipboard(srcPath);

      return {.originalPath = srcPath, .newPath = duplicateResult.newPath};
    }
  }

  if (QFile::exists(destPath)) {
    // If the file exists, treat it as a duplicate
    // operation.
    const auto duplicateResult = duplicate(destPath);

    // Convert the `DuplicateResult` to a `PasteItem`.
    return {.originalPath = destPath, .newPath = duplicateResult.newPath};
  }

  // Regular copy operation on a single file.
  if (!QFile::copy(srcPath, destPath)) {
    qDebug() << "Failed to copy file:" << srcPath;
  }

  return item;
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

  // Remove the original item from the clipboard.
  removeFromClipboard(itemPath);

  return true;
}

bool FileIoService::copyRecursively(const QString &sourceFolder,
                                    const QString &destFolder) {
  QDir sourceDir(sourceFolder);
  QDir destDir(destFolder);

  if (!sourceDir.exists()) {
    return false;
  }

  QString absSrc = QFileInfo(sourceFolder).absoluteFilePath();
  QString absDest = QFileInfo(destFolder).absoluteFilePath();

  // Prevent copying a folder into itself.
  if (absSrc == absDest) {
    return false;
  }

  // Prevent copying a folder into its own subdirectory.
  QString srcBoundary = absSrc + QDir::separator();

  if (absDest.startsWith(srcBoundary)) {
    return false;
  }

  // Make the destination directory if it does not exist.
  if (!destDir.exists()) {
    destDir.mkpath(".");
  }

  // Get the list of items in the source directory.
  QStringList items = sourceDir.entryList(QDir::Files | QDir::Dirs |
                                          QDir::NoDotAndDotDot | QDir::Hidden);

  // NOLINTNEXTLINE(readability-use-anyofallof)
  for (const QString &item : items) {
    QString srcName = sourceFolder + QDir::separator() + item;
    QString destName = destFolder + QDir::separator() + item;

    QFileInfo info(srcName);

    if (info.isDir()) {
      if (!copyRecursively(srcName, destName)) {
        return false;
      }
    } else {
      if (!QFile::copy(srcName, destName)) {
        qDebug() << "Failed to copy file:" << srcName
                 << " to destination:" << destName;
      }
    }
  }

  return true;
}
