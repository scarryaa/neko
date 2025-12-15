#ifndef EDITOR_CONTROLLER_H
#define EDITOR_CONTROLLER_H

#include <QObject>
#include <neko-core/src/ffi/mod.rs.h>

class EditorController : public QObject {
  Q_OBJECT

public:
  explicit EditorController();
  ~EditorController();

signals:
  void cursorChanged(int row, int col, int cursorCount, int selectionCount);
  void selectionChanged(int selectionCount);
  void lineCountChanged(int lineCount);
  void bufferChanged();
  void viewportChanged();

public slots:
  void setEditor(neko::Editor *editor);
  void applyChangeSet(const neko::ChangeSetFfi &changeSet);

  void moveTo();
  void insertText();
  void backspace();
  void deleteForwards();

  void refresh();

private:
  neko::Editor *editor;
};

#endif
