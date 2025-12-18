#include "editor_controller.h"

EditorController::EditorController(neko::Editor *editor) {}

EditorController::~EditorController() {}

void EditorController::setEditor(neko::Editor *editor) {
  this->editor = editor;
}

void EditorController::applyChangeSet(const neko::ChangeSetFfi &changeSet) {
  if (!editor) {
    return;
  }

  const uint32_t m = changeSet.mask;

  if (m & ChangeMask::Selection) {
    auto selectionCount = editor->get_number_of_selections();
    emit selectionChanged(selectionCount);
  }

  if (m & (ChangeMask::Viewport | ChangeMask::LineCount | ChangeMask::Widths)) {
    emit viewportChanged();
  }

  if (m & ChangeMask::LineCount) {
    int lineCount = editor->get_line_count();
    emit lineCountChanged(lineCount);
  }

  if (m & ChangeMask::Cursor) {
    auto lastAddedCursor = editor->get_last_added_cursor();
    auto [clampedRow, clampedColumn] =
        normalizeCursorPosition(lastAddedCursor.row, lastAddedCursor.col);
    auto cursorCount = editor->get_cursor_positions().size();
    auto selectionCount = editor->get_number_of_selections();

    emit cursorChanged(clampedRow, clampedColumn, cursorCount, selectionCount);
  }

  if (m & ChangeMask::Buffer) {
    emit bufferChanged();
  }
}

void EditorController::nav(neko::ChangeSetFfi (neko::Editor::*moveFn)(),
                           neko::ChangeSetFfi (neko::Editor::*selectFn)(),
                           bool shouldSelect) {
  if (!editor)
    return;

  if (shouldSelect) {
    do_op(selectFn);
  } else {
    do_op(moveFn);
  }
}

void EditorController::moveTo(int row, int column, bool clearSelection) {
  do_op(&neko::Editor::move_to, row, column, clearSelection);
}

void EditorController::selectTo(int row, int column) {
  do_op(&neko::Editor::select_to, row, column);
}

void EditorController::moveOrSelectLeft(bool shouldSelect) {
  nav(&neko::Editor::move_left, &neko::Editor::select_left, shouldSelect);
}

void EditorController::moveOrSelectRight(bool shouldSelect) {
  nav(&neko::Editor::move_right, &neko::Editor::select_right, shouldSelect);
}

void EditorController::moveOrSelectUp(bool shouldSelect) {
  nav(&neko::Editor::move_up, &neko::Editor::select_up, shouldSelect);
}

void EditorController::moveOrSelectDown(bool shouldSelect) {
  nav(&neko::Editor::move_down, &neko::Editor::select_down, shouldSelect);
}

void EditorController::insertText(std::string text) {
  if (text.empty())
    return;

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
  if (!editor)
    return;

  auto selection = editor->get_selection();
  if (!selection.active)
    return;

  auto rawText = editor->copy();
  if (rawText.empty())
    return;

  QApplication::clipboard()->setText(QString::fromUtf8(rawText));
}

void EditorController::paste() {
  QString text = QApplication::clipboard()->text();
  do_op(&neko::Editor::paste, text.toStdString());
}

void EditorController::cut() {
  if (!editor->has_active_selection()) {
    return;
  }

  auto rawText = editor->copy();
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

  auto cursorPosition =
      editor->get_cursor_positions()[editor->get_active_cursor_index()];
  auto [normalizedRow, normalizedCol] =
      normalizeCursorPosition(cursorPosition.row, cursorPosition.col);

  auto cursorCount = editor->get_cursor_positions().size();
  auto selectionCount = editor->get_number_of_selections();
  emit cursorChanged(normalizedRow, normalizedCol, cursorCount, selectionCount);

  emit viewportChanged();
}

void EditorController::removeCursor(int row, int col) {
  editor->remove_cursor(row, col);

  auto cursorPosition =
      editor->get_cursor_positions()[editor->get_active_cursor_index()];
  auto [normalizedRow, normalizedCol] =
      normalizeCursorPosition(cursorPosition.row, cursorPosition.col);

  auto cursorCount = editor->get_cursor_positions().size();
  auto selectionCount = editor->get_number_of_selections();
  emit cursorChanged(normalizedRow, normalizedCol, cursorCount, selectionCount);

  emit viewportChanged();
}

void EditorController::refresh() {
  // TODO: Emit current state after change (e.g. opening a file)
}

template <typename Fn, typename... Args>
void EditorController::do_op(Fn &&fn, Args &&...args) {
  if (!editor)
    return;

  auto changeSet =
      std::invoke(std::forward<Fn>(fn), editor, std::forward<Args>(args)...);
  applyChangeSet(changeSet);
}

std::pair<int, int> EditorController::normalizeCursorPosition(int row,
                                                              int col) const {
  if (!editor) {
    return {row, col};
  }

  int lineCount = static_cast<int>(editor->get_line_count());
  int clampedRow = std::clamp(row, 0, std::max(0, lineCount - 1));
  int lineLength = static_cast<int>(editor->get_line_length(clampedRow));
  int clampedCol = std::clamp(col, 0, std::max(0, lineLength));

  return {clampedRow, clampedCol};
}
