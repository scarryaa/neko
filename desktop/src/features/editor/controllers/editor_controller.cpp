#include "editor_controller.h"
#include "features/editor/change_mask.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QClipboard>

EditorController::EditorController(neko::Editor *editor) : editor(editor) {}

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

neko::Selection EditorController::getSelection() const {
  return editor->get_selection();
}

std::vector<neko::CursorPosition> EditorController::getCursorPositions() const {
  std::vector<neko::CursorPosition> cursors;
  const auto rawCursors = editor->get_cursor_positions();

  for (auto cursor : rawCursors) {
    cursors.push_back(cursor);
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

neko::CursorPosition EditorController::getLastAddedCursor() const {
  return editor->get_last_added_cursor();
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
  do_op(&neko::Editor::select_word, row, column);
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
                                      const int anchorStartCol,
                                      const int anchorEndRow,
                                      const int anchorEndCol, const int row,
                                      const int col) {
  do_op(&neko::Editor::select_word_drag, anchorStartRow, anchorStartCol,
        anchorEndRow, anchorEndCol, row, col);
}

void EditorController::selectLineDrag(int anchorRow, int row) {
  do_op(&neko::Editor::select_line_drag, anchorRow, row);
}

void EditorController::moveTo(const int row, const int column,
                              const bool clearSelection) {
  do_op(&neko::Editor::move_to, row, column, clearSelection);
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

void EditorController::selectTo(const int row, const int column) {
  do_op(&neko::Editor::select_to, row, column);
}

void EditorController::insertText(const std::string &text) {
  if (text.empty()) {
    return;
  }

  do_op(&neko::Editor::insert_text, text);
}

void EditorController::insertNewline() { do_op(&neko::Editor::insert_newline); }

void EditorController::insertTab() { do_op(&neko::Editor::insert_tab); }

void EditorController::backspace() { do_op(&neko::Editor::backspace); }

void EditorController::deleteForwards() {
  do_op(&neko::Editor::delete_forwards);
}

void EditorController::selectAll() { do_op(&neko::Editor::select_all); }

void EditorController::copy() {
  if (editor == nullptr) {
    return;
  }

  const auto selection = editor->get_selection();
  if (!selection.active) {
    return;
  }

  const auto rawText = editor->copy();
  if (rawText.empty()) {
    return;
  }

  QApplication::clipboard()->setText(QString::fromUtf8(rawText));
}

void EditorController::paste() {
  const QString text = QApplication::clipboard()->text();
  do_op(&neko::Editor::paste, text.toStdString());
}

void EditorController::cut() {
  if (!editor->has_active_selection()) {
    return;
  }

  const auto rawText = editor->copy();
  if (rawText.empty()) {
    return;
  }

  QApplication::clipboard()->setText(QString::fromUtf8(rawText));
  do_op(&neko::Editor::delete_forwards);
}

void EditorController::undo() { do_op(&neko::Editor::undo); }

void EditorController::redo() { do_op(&neko::Editor::redo); }

void EditorController::clearSelectionOrCursors() {
  if (editor->has_active_selection()) {
    do_op(&neko::Editor::clear_selection);
  } else {
    do_op(&neko::Editor::clear_cursors);
  }
}

// NOLINTNEXTLINE
void EditorController::addCursor(neko::AddCursorDirectionKind dirKind, int row,
                                 int column) {
  switch (dirKind) {
  case neko::AddCursorDirectionKind::Above: {
    neko::AddCursorDirectionFfi dir;
    dir.kind = dirKind;

    editor->add_cursor(dir);
    break;
  }
  case neko::AddCursorDirectionKind::Below: {
    neko::AddCursorDirectionFfi dir;
    dir.kind = dirKind;

    editor->add_cursor(dir);
    break;
  }
  case neko::AddCursorDirectionKind::At: {
    neko::AddCursorDirectionFfi dir;
    dir.kind = dirKind;
    dir.row = row;
    dir.col = column;

    editor->add_cursor(dir);
    break;
  }
  }

  const auto cursorPosition =
      editor->get_cursor_positions()[editor->get_active_cursor_index()];
  const auto [normalizedRow, normalizedCol] =
      normalizeCursorPosition(static_cast<int>(cursorPosition.row),
                              static_cast<int>(cursorPosition.col));

  const auto cursorCount = editor->get_cursor_positions().size();
  const auto selectionCount = editor->get_number_of_selections();
  emit cursorChanged(normalizedRow, normalizedCol,
                     static_cast<int>(cursorCount),
                     static_cast<int>(selectionCount));

  emit viewportChanged();
}

void EditorController::removeCursor(const int row, const int col) {
  editor->remove_cursor(row, col);

  const auto cursorPosition =
      editor->get_cursor_positions()[editor->get_active_cursor_index()];
  const auto [normalizedRow, normalizedCol] =
      normalizeCursorPosition(static_cast<int>(cursorPosition.row),
                              static_cast<int>(cursorPosition.col));

  const auto cursorCount = editor->get_cursor_positions().size();
  const auto selectionCount = editor->get_number_of_selections();
  emit cursorChanged(normalizedRow, normalizedCol,
                     static_cast<int>(cursorCount),
                     static_cast<int>(selectionCount));

  emit viewportChanged();
}

void EditorController::refresh() {
  // TODO(scarlet): Emit current state after change (e.g. opening a file)
}

void EditorController::applyChangeSet(const neko::ChangeSetFfi &changeSet) {
  if (editor == nullptr) {
    return;
  }

  const uint32_t mask = changeSet.mask;

  if ((mask & ChangeMask::Selection) != 0U) {
    const auto selectionCount = editor->get_number_of_selections();
    emit selectionChanged(static_cast<int>(selectionCount));
  }

  if ((mask & (ChangeMask::Viewport | ChangeMask::LineCount |
               ChangeMask::Widths)) != 0U) {
    emit viewportChanged();
  }

  if ((mask & ChangeMask::LineCount) != 0U) {
    const int lineCount = static_cast<int>(editor->get_line_count());
    emit lineCountChanged(lineCount);
  }

  if ((mask & ChangeMask::Cursor) != 0U) {
    const auto lastAddedCursor = editor->get_last_added_cursor();
    const auto [clampedRow, clampedColumn] =
        normalizeCursorPosition(static_cast<int>(lastAddedCursor.row),
                                static_cast<int>(lastAddedCursor.col));
    const auto cursorCount = editor->get_cursor_positions().size();
    const auto selectionCount = editor->get_number_of_selections();

    emit cursorChanged(clampedRow, clampedColumn, static_cast<int>(cursorCount),
                       static_cast<int>(selectionCount));
  }

  if ((mask & ChangeMask::Buffer) != 0U) {
    emit bufferChanged();
  }
}

void EditorController::nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
                           neko::ChangeSetFfi (neko::Editor::*selectFn)(),
                           const bool shouldSelect) {
  if (editor == nullptr) {
    return;
  }

  if (shouldSelect) {
    do_op(selectFn);
  } else {
    do_op(moveFn);
  }
}

template <typename Fn, typename... Args>
void EditorController::do_op(Fn &&function, Args &&...args) {
  if (!editor) {
    return;
  }

  const auto changeSet = std::invoke(std::forward<const Fn>(function), editor,
                                     std::forward<const Args>(args)...);
  applyChangeSet(changeSet);
}

std::pair<const int, const int>
EditorController::normalizeCursorPosition(const int row, const int col) const {
  if (editor == nullptr) {
    return {row, col};
  }

  const int lineCount = static_cast<int>(editor->get_line_count());
  const int clampedRow = std::clamp(row, 0, std::max(0, lineCount - 1));
  const int lineLength = static_cast<int>(editor->get_line_length(clampedRow));
  const int clampedCol = std::clamp(col, 0, std::max(0, lineLength));

  return {clampedRow, clampedCol};
}
