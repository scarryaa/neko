#ifndef EDITOR_CONTROLLER_H
#define EDITOR_CONTROLLER_H

#include "features/editor/types/types.h"
#include "types/ffi_types_fwd.h"
#include <QObject>
#include <QString>
#include <QStringList>

class EditorController : public QObject {
  Q_OBJECT

public:
  struct EditorControllerProps {
    neko::Editor *editor;
  };

  explicit EditorController(const EditorControllerProps &props);
  ~EditorController() override = default;

  // Getters
  [[nodiscard]] bool isEmpty() const;
  [[nodiscard]] QString getLine(int index) const;
  [[nodiscard]] QStringList getLines() const;
  [[nodiscard]] int getLineCount() const;
  [[nodiscard]] Selection getSelection() const;
  [[nodiscard]] std::vector<Cursor> getCursorPositions() const;
  [[nodiscard]] bool needsWidthMeasurement(int index) const;
  [[nodiscard]] double getMaxWidth() const;
  [[nodiscard]] bool cursorExistsAt(int row, int column) const;
  [[nodiscard]] bool bufferIsEmpty() const;
  [[nodiscard]] int getNumberOfSelections() const;
  [[nodiscard]] Cursor getLastAddedCursor() const;
  [[nodiscard]] int getLineLength(int index) const;

  // Setters
  void setLineWidth(int index, double width);
  void setEditor(neko::Editor *editor);

  // Selection/Cursor movement
  void selectWord(int row, int column);
  void selectLine(int row);
  void selectWordDrag(int anchorStartRow, int anchorStartColumn,
                      int anchorEndRow, int anchorEndColumn, int row,
                      int column);
  void selectLineDrag(int anchorRow, int row);
  void selectTo(int row, int column);
  void moveOrSelectLeft(bool shouldSelect);
  void moveOrSelectRight(bool shouldSelect);
  void moveOrSelectUp(bool shouldSelect);
  void moveOrSelectDown(bool shouldSelect);
  void moveTo(int row, int column, bool clearSelection);

  // Buffer manipulation
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
  void addCursor(neko::AddCursorDirectionKind directionKind, int row = 0,
                 int column = 0);
  void removeCursor(int row, int column);

  void applyChangeSet(const neko::ChangeSetFfi &changeSet);

signals:
  void cursorChanged(int row, int column, int cursorCount, int selectionCount);
  void selectionChanged(int selectionCount);
  void lineCountChanged(int lineCount);
  void bufferChanged();
  void viewportChanged();

private:
  // Helpers
  void emitCursorAndSelection();
  void emitSelectionOnly();
  void emitLineCountChanged();

  void copyToClipboardAndMaybeDelete(bool deleteAfter);

  [[nodiscard]] neko::AddCursorDirectionFfi static makeCursorDirection(
      neko::AddCursorDirectionKind kind, int row = 0, int column = 0);
  [[nodiscard]] std::pair<int, int> normalizeCursorPosition(int row,
                                                            int column) const;

  static inline bool hasFlag(uint32_t mask, uint32_t flag);

  template <typename Fn, typename... Args>
  void doOp(Fn &&function, Args &&...args);
  void nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
           neko::ChangeSetFfi (neko::Editor::*selectFn)(), bool shouldSelect);

  neko::Editor *editor;
};

#endif
