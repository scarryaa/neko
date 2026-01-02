#include "editor_controller.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QClipboard>

EditorBridge::EditorBridge(EditorBridgeProps props)
    : editorController(std::move(props.editorController)) {}

bool EditorController::isEmpty() const { return editor->buffer_is_empty(); }

QString EditorController::getLine(const int index) const {
  return QString::fromUtf8(editor->get_line(index));
}

QStringList EditorController::getLines() const {
  const auto rawLines = editor->get_lines();
  QStringList lines = QStringList();

  for (const auto &line : rawLines) {
    lines.append(QString::fromUtf8(line));
  }

  return lines;
}

int EditorController::getLineCount() const {
  return static_cast<int>(editor->get_line_count());
}

Selection EditorController::getSelection() const {
  const auto selection = editor->get_selection();
  return {
      {static_cast<int>(selection.start.row),
       static_cast<int>(selection.start.col)},
      {static_cast<int>(selection.end.row),
       static_cast<int>(selection.end.col)},
      {static_cast<int>(selection.anchor.row),
       static_cast<int>(selection.anchor.col)},
      selection.active,
  };
}

std::vector<Cursor> EditorController::getCursorPositions() const {
  std::vector<Cursor> cursors;
  const auto nekoCursors = editor->get_cursor_positions();
  cursors.reserve(nekoCursors.size());

  for (auto cursor : nekoCursors) {
    cursors.push_back(
        {static_cast<int>(cursor.row), static_cast<int>(cursor.col)});
  }
  return cursors;
}

bool EditorController::needsWidthMeasurement(const int index) const {
  return editor->needs_width_measurement(index);
}

double EditorController::getMaxWidth() const { return editor->get_max_width(); }

bool EditorController::cursorExistsAt(const int row, const int column) const {
  return editor->cursor_exists_at(row, column);
}

bool EditorController::bufferIsEmpty() const {
  return editor->buffer_is_empty();
}

int EditorController::getNumberOfSelections() const {
  return static_cast<int>(editor->get_number_of_selections());
}

Cursor EditorController::getLastAddedCursor() const {
  const auto nekoCursor = editor->get_last_added_cursor();
  const auto cursor = Cursor{.row = static_cast<int>(nekoCursor.row),
                             .column = static_cast<int>(nekoCursor.col)};

  return cursor;
}

int EditorController::getLineLength(int index) const {
  return static_cast<int>(editor->get_line_length(index));
}

void EditorController::setLineWidth(const int index, const double width) {
  editor->set_line_width(index, width);
}

void EditorController::setEditor(neko::Editor *editor) {
  this->editor = editor;
}

void EditorController::selectWord(const int row, const int column) {
  doOp(&neko::Editor::select_word, row, column);
}

void EditorController::selectLine(int row) {
  if (editor == nullptr) {
    return;
  }

  const int lineCount = static_cast<int>(editor->get_line_count());
  if (lineCount == 0) {
    return;
  }

  const int maxRow = std::max(0, lineCount - 1);
  const int clampedRow = std::clamp(row, 0, maxRow);

  moveTo(clampedRow, 0, true);

  if (clampedRow + 1 < lineCount) {
    selectTo(clampedRow + 1, 0);
  } else {
    const int lineLength =
        static_cast<int>(editor->get_line_length(clampedRow));
    selectTo(clampedRow, lineLength);
  }
}

void EditorController::selectWordDrag(const int anchorStartRow,
                                      const int anchorStartColumn,
                                      const int anchorEndRow,
                                      const int anchorEndColumn, const int row,
                                      const int column) {
  doOp(&neko::Editor::select_word_drag, anchorStartRow, anchorStartColumn,
       anchorEndRow, anchorEndColumn, row, column);
}

void EditorController::selectLineDrag(int anchorRow, int row) {
  doOp(&neko::Editor::select_line_drag, anchorRow, row);
}

void EditorController::selectTo(const int row, const int column) {
  doOp(&neko::Editor::select_to, row, column);
}

void EditorController::moveOrSelectLeft(const bool shouldSelect) {
  nav(&neko::Editor::move_left, &neko::Editor::select_left, shouldSelect);
}

void EditorController::moveOrSelectRight(const bool shouldSelect) {
  nav(&neko::Editor::move_right, &neko::Editor::select_right, shouldSelect);
}

void EditorController::moveOrSelectUp(const bool shouldSelect) {
  nav(&neko::Editor::move_up, &neko::Editor::select_up, shouldSelect);
}

void EditorController::moveOrSelectDown(const bool shouldSelect) {
  nav(&neko::Editor::move_down, &neko::Editor::select_down, shouldSelect);
}

void EditorController::moveTo(const int row, const int column,
                              const bool clearSelection) {
  doOp(&neko::Editor::move_to, row, column, clearSelection);
}

void EditorController::insertText(const std::string &text) {
  if (text.empty()) {
    return;
  }

  doOp(&neko::Editor::insert_text, text);
}

void EditorController::insertNewline() { doOp(&neko::Editor::insert_newline); }

void EditorController::insertTab() { doOp(&neko::Editor::insert_tab); }

void EditorController::backspace() { doOp(&neko::Editor::backspace); }

void EditorController::deleteForwards() {
  doOp(&neko::Editor::delete_forwards);
}

void EditorController::selectAll() { doOp(&neko::Editor::select_all); }

void EditorController::copy() { copyToClipboardAndMaybeDelete(false); }

void EditorController::cut() { copyToClipboardAndMaybeDelete(true); }

void EditorController::paste() {
  const QString text = QApplication::clipboard()->text();
  doOp(&neko::Editor::paste, text.toStdString());
}

void EditorController::undo() { doOp(&neko::Editor::undo); }

void EditorController::redo() { doOp(&neko::Editor::redo); }

void EditorController::clearSelectionOrCursors() {
  if (editor->has_active_selection()) {
    doOp(&neko::Editor::clear_selection);
  } else {
    doOp(&neko::Editor::clear_cursors);
  }
}

// NOLINTNEXTLINE
void EditorController::addCursor(neko::AddCursorDirectionKind directionKind,
                                 int row, int column) {
  if (editor == nullptr) {
    return;
  }

  const auto direction =
      EditorController::makeCursorDirection(directionKind, row, column);
  editor->add_cursor(direction);

  emitCursorAndSelection();
  emit viewportChanged();
}

void EditorController::removeCursor(int row, int column) {
  if (editor == nullptr) {
    return;
  }

  editor->remove_cursor(row, column);

  emitCursorAndSelection();
  emit viewportChanged();
}

void EditorController::applyChangeSet(const neko::ChangeSetFfi &changeSet) {
  if (editor == nullptr) {
    return;
  }

  const auto mask = changeSet.mask;

  if (hasFlag(mask, ChangeMask::Selection)) {
    emitSelectionOnly();
  }

  if (hasFlag(mask, (ChangeMask::Viewport | ChangeMask::LineCount |
                     ChangeMask::Widths))) {
    emit viewportChanged();
  }

  if (hasFlag(mask, ChangeMask::LineCount)) {
    emitLineCountChanged();
  }

  if (hasFlag(mask, ChangeMask::Cursor)) {
    emitCursorAndSelection();
  }

  if (hasFlag(mask, ChangeMask::Buffer)) {
    emit bufferChanged();
  }
}

void EditorController::emitCursorAndSelection() {
  if (editor == nullptr) {
    return;
  }

  const auto cursors = editor->get_cursor_positions();
  if (cursors.empty()) {
    return;
  }

  const auto activeIndex = editor->get_active_cursor_index();
  const auto &active = cursors[activeIndex];
  const auto [row, col] = normalizeCursorPosition(static_cast<int>(active.row),
                                                  static_cast<int>(active.col));

  const auto cursorCount = static_cast<int>(cursors.size());
  const auto selectionCount =
      static_cast<int>(editor->get_number_of_selections());

  emit cursorChanged(row, col, cursorCount, selectionCount);
}

void EditorController::emitSelectionOnly() {
  if (editor == nullptr) {
    return;
  }

  const auto selectionCount =
      static_cast<int>(editor->get_number_of_selections());
  emit selectionChanged(selectionCount);
}

void EditorController::emitLineCountChanged() {
  if (editor == nullptr) {
    return;
  }

  const int lineCount = static_cast<int>(editor->get_line_count());
  emit lineCountChanged(lineCount);
}

void EditorController::copyToClipboardAndMaybeDelete(bool deleteAfter) {
  if (editor == nullptr) {
    return;
  }

  if (!editor->has_active_selection()) {
    return;
  }

  const auto rawText = editor->copy();
  if (rawText.empty()) {
    return;
  }

  QApplication::clipboard()->setText(QString::fromUtf8(rawText));

  if (deleteAfter) {
    doOp(&neko::Editor::delete_forwards);
  }
}

neko::AddCursorDirectionFfi
EditorController::makeCursorDirection(neko::AddCursorDirectionKind kind,
                                      // NOLINTNEXTLINE
                                      int row, int column) {
  neko::AddCursorDirectionFfi direction;
  direction.kind = kind;
  direction.row = 0;
  direction.col = 0;

  if (kind == neko::AddCursorDirectionKind::At) {
    direction.row = row;
    direction.col = column;
  }

  return direction;
}

std::pair<int, int>
EditorController::normalizeCursorPosition(const int row,
                                          const int column) const {
  if (editor == nullptr) {
    return {row, column};
  }

  const int lineCount = static_cast<int>(editor->get_line_count());
  const int clampedRow = std::clamp(row, 0, std::max(0, lineCount - 1));
  const int lineLength = static_cast<int>(editor->get_line_length(clampedRow));
  const int clampedCol = std::clamp(column, 0, std::max(0, lineLength));

  return {clampedRow, clampedCol};
}

inline bool EditorController::hasFlag(uint32_t mask, uint32_t flag) {
  return (mask & flag) != 0U;
}

template <typename Fn, typename... Args>
void EditorController::doOp(Fn &&function, Args &&...args) {
  if (!editor) {
    return;
  }

  const auto changeSet = std::invoke(std::forward<Fn>(function), editor,
                                     std::forward<Args>(args)...);
  applyChangeSet(changeSet);
}

void EditorController::nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
                           neko::ChangeSetFfi (neko::Editor::*selectFn)(),
                           bool shouldSelect) {
  if (editor == nullptr) {
    return;
  }

  if (shouldSelect) {
    doOp(selectFn);
  } else {
    doOp(moveFn);
  }
}
