#ifndef DIALOG_SERVICE_H
#define DIALOG_SERVICE_H

#include "features/main_window/interfaces/close_decision.h"
#include "types/qt_types_fwd.h"
#include <QList>
#include <QObject>
#include <QString>
#include <cstdint>
#include <optional>

QT_FWD(QWidget)

/// \class DialogService
/// \brief Handles showing any dialogs that are needed (selecting a file
/// explorer root directory, 'saveAs' dialogs, etc.)
class DialogService : public QObject {
  Q_OBJECT

public:
  enum class OperationType : uint8_t {
    NewFile,
    NewDirectory,
    RenameFile,
    RenameDirectory
  };

  explicit DialogService(QObject *parent = nullptr);
  ~DialogService() override = default;

  [[nodiscard]] static QString openDirectorySelectionDialog(QWidget *parent);
  [[nodiscard]] static QString
  openFileSelectionDialog(const std::optional<QString> &initialDirectory,
                          QWidget *parent);
  static CloseDecision openCloseConfirmationDialog(const QList<int> &ids,
                                                   int modifiedCount,
                                                   QWidget *parent);
  static QString openSaveAsDialog(std::optional<QString> initialDirectory,
                                  std::optional<QString> initialFileName,
                                  QWidget *parent);
  static QString openItemNameDialog(QWidget *parent, const OperationType &type,
                                    const QString &initialText = "");
};

#endif
