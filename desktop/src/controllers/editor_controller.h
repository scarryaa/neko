#ifndef EDITOR_CONTROLLER_H
#define EDITOR_CONTROLLER_H

#include "utils/change_mask.h"
#include <QApplication>
#include <QClipboard>
#include <QObject>
#include <functional>
#include <neko-core/src/ffi/mod.rs.h>

class EditorController : public QObject {
  Q_OBJECT

public:
  explicit EditorController(neko::Editor *editor);
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

  void moveTo(int row, int column, bool clearSelection);
  void moveOrSelectLeft(bool shouldSelect);
  void moveOrSelectRight(bool shouldSelect);
  void moveOrSelectUp(bool shouldSelect);
  void moveOrSelectDown(bool shouldSelect);

  void insertText(std::string text);
  void insertNewline();
  void insertTab();
  void backspace();
  void deleteForwards();
  void selectAll();
  void copy();
  void paste();
  void cut();

  void undo();
  void redo();

  void clearSelectionOrCursors();
  void addCursor(neko::AddCursorDirectionKind dirKind, int row = 0,
                 int col = 0);

  void refresh();

private:
  void nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
           neko::ChangeSetFfi (neko::Editor::*selectFn)(), bool shouldSelect);
  template <typename Fn, typename... Args> void do_op(Fn &&fn, Args &&...args);
  void do_op(std::function<neko::ChangeSetFfi()> f);

  neko::Editor *editor;
};

#endif
