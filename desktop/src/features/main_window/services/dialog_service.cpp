#include "dialog_service.h"
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QPushButton>
#include <QWidget>

DialogService::DialogService(QObject *parent) : QObject(parent) {}

QString DialogService::promptFileExplorerDirectory(QWidget *parent) {
  return QFileDialog::getExistingDirectory(
      parent, tr("Select a directory"), QDir::homePath(),
      QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
}

QString DialogService::openFile(const std::optional<QString> &initialDirectory,
                                QWidget *parent) {
  QString baseDir;

  if (initialDirectory && !initialDirectory->isEmpty()) {
    QFileInfo info(*initialDirectory);
    baseDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  } else {
    baseDir = QDir::homePath();
  }

  return QFileDialog::getOpenFileName(parent, tr("Open a file"), baseDir);
}

CloseDecision DialogService::showTabCloseConfirmationDialog(
    const QList<int> &ids, int modifiedCount, QWidget *parent) {
  if (ids.isEmpty() || modifiedCount == 0) {
    return CloseDecision::DontSave;
  }

  const bool multipleModifiedTabs = modifiedCount > 1;
  QMessageBox box(QMessageBox::Warning, tr("Close Tabs"),
                  tr("%1 tab%2 unsaved edits.")
                      .arg(modifiedCount)
                      .arg(multipleModifiedTabs ? "s have" : " has"),
                  QMessageBox::NoButton, parent);

  auto *saveBtn = box.addButton(tr(multipleModifiedTabs ? "Save all" : "Save"),
                                QMessageBox::AcceptRole);
  auto *dontSaveBtn =
      box.addButton(tr(multipleModifiedTabs ? "Discard all" : "Don't Save"),
                    QMessageBox::DestructiveRole);
  auto *cancelBtn = box.addButton(QMessageBox::Cancel);

  box.setDefaultButton(cancelBtn);
  box.setEscapeButton(cancelBtn);

  box.exec();

  if (box.clickedButton() == saveBtn) {
    return CloseDecision::Save;
  }

  if (box.clickedButton() == dontSaveBtn) {
    return CloseDecision::DontSave;
  }

  return CloseDecision::Cancel;
}

QString DialogService::promptSaveAsPath(std::optional<QString> initialDirectory,
                                        std::optional<QString> initialFileName,
                                        QWidget *parent) {
  QString baseDir;

  if (initialDirectory && !initialDirectory->isEmpty()) {
    QFileInfo info(*initialDirectory);

    baseDir = info.isDir() ? info.absoluteFilePath() : info.absolutePath();
  } else {
    baseDir = QDir::homePath();
  }

  QString initialPath = baseDir;
  if (initialFileName && !initialFileName->isEmpty()) {
    initialPath = QDir(baseDir).filePath(*initialFileName);
  }

  return QFileDialog::getSaveFileName(parent, tr("Save As"), initialPath);
}
