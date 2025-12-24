#ifndef EDITOR_CONTROLLER_H
#define EDITOR_CONTROLLER_H

#include <QApplication>
#include <QClipboard>
#include <QObject>
#include <QString>
#include <QStringList>
#include <neko-core/src/ffi/bridge.rs.h>

class EditorController : public QObject {
  Q_OBJECT

public:
  explicit EditorController(neko::Editor *editor);
  ~EditorController() override = default;

  // Getters
  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] QString getLine(int index) const;
  [[nodiscard]] QStringList getLines() const;
  [[nodiscard]] int getLineCount() const;
  [[nodiscard]] neko::Selection getSelection() const;
  [[nodiscard]] std::vector<neko::CursorPosition> getCursorPositions() const;
  [[nodiscard]] bool needsWidthMeasurement(int index) const;
  [[nodiscard]] double getMaxWidth() const;
  [[nodiscard]] bool cursorExistsAt(int row, int column) const;
  [[nodiscard]] bool bufferIsEmpty() const;
  [[nodiscard]] int getNumberOfSelections() const;
  [[nodiscard]] neko::CursorPosition getLastAddedCursor() const;
  [[nodiscard]] int getLineLength(int index) const;

  // Setters
  void setLineWidth(int index, double width);
  void setEditor(neko::Editor *editor);
  void selectWord(int row, int column);
  void selectLine(int row);
  void selectWordDrag(int anchorStartRow, int anchorStartCol, int anchorEndRow,
                      int anchorEndCol, int row, int col);
  void selectLineDrag(int anchorRow, int row);
  void moveTo(int row, int column, bool clearSelection);
  void moveOrSelectLeft(bool shouldSelect);
  void moveOrSelectRight(bool shouldSelect);
  void moveOrSelectUp(bool shouldSelect);
  void moveOrSelectDown(bool shouldSelect);
  void selectTo(int row, int column);

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
  void addCursor(neko::AddCursorDirectionKind dirKind, int row = 0,
                 int column = 0);
  void removeCursor(int row, int col);
  void refresh();
  void applyChangeSet(const neko::ChangeSetFfi &changeSet);

signals:
  void cursorChanged(int row, int col, int cursorCount, int selectionCount);
  void selectionChanged(int selectionCount);
  void lineCountChanged(int lineCount);
  void bufferChanged();
  void viewportChanged();

private:
  void nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
           neko::ChangeSetFfi (neko::Editor::*selectFn)(), bool shouldSelect);
  template <typename Fn, typename... Args>
  void do_op(Fn &&function, Args &&...args);
  void do_op(std::function<neko::ChangeSetFfi()> function);
  [[nodiscard]] std::pair<const int, const int>
  normalizeCursorPosition(int row, int col) const;

  neko::Editor *editor;
};

#endif
