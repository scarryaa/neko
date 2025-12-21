#ifndef EDITOR_CONTROLLER_H
#define EDITOR_CONTROLLER_H

#include "utils/change_mask.h"
#include <QApplication>
#include <QClipboard>
#include <QObject>
#include <neko-core/src/ffi/mod.rs.h>

class EditorController : public QObject {
  Q_OBJECT

public:
  explicit EditorController(neko::Editor *editor);
  ~EditorController();

  // Getters
  const bool isEmpty() const;
  const QString getLine(const int index) const;
  const QStringList getLines() const;
  const int getLineCount() const;
  const neko::Selection getSelection() const;
  const std::vector<neko::CursorPosition> getCursorPositions() const;
  const bool needsWidthMeasurement(const int index) const;
  const double getMaxWidth() const;
  void setLineWidth(const int index, const double width);
  const bool cursorExistsAt(const int row, const int column) const;
  const bool bufferIsEmpty() const;
  const int getNumberOfSelections() const;

  // Setters
  void selectWord(const int row, const int column);
  void selectLine(const int row);
  void selectWordDrag(const int anchorStartRow, const int anchorStartCol,
                      const int anchorEndRow, const int anchorEndCol,
                      const int row, const int col);
  void selectLineDrag(const int anchorRow, const int row);
  void moveTo(const int row, const int column, const bool clearSelection);
  void moveOrSelectLeft(const bool shouldSelect);
  void moveOrSelectRight(const bool shouldSelect);
  void moveOrSelectUp(const bool shouldSelect);
  void moveOrSelectDown(const bool shouldSelect);
  void selectTo(const int row, const int column);

  void insertText(const std::string &text);
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
  void addCursor(const neko::AddCursorDirectionKind dirKind, const int row = 0,
                 const int col = 0);
  void removeCursor(const int row, const int col);
  void refresh();
  void applyChangeSet(const neko::ChangeSetFfi &changeSet);

signals:
  void cursorChanged(const int row, const int col, const int cursorCount,
                     const int selectionCount);
  void selectionChanged(const int selectionCount);
  void lineCountChanged(const int lineCount);
  void bufferChanged();
  void viewportChanged();

public slots:
  void setEditor(neko::Editor *editor);

private:
  void nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
           neko::ChangeSetFfi (neko::Editor::*selectFn)(),
           const bool shouldSelect);
  template <typename Fn, typename... Args> void do_op(Fn &&fn, Args &&...args);
  void do_op(const std::function<neko::ChangeSetFfi()> f);
  std::pair<const int, const int> normalizeCursorPosition(const int row,
                                                          const int col) const;

  neko::Editor *editor;
};

#endif
