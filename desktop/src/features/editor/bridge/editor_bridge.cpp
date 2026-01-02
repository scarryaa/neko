#include "editor_bridge.h"
#include "neko-core/src/ffi/bridge.rs.h"
#include <QApplication>
#include <QClipboard>

EditorBridge::EditorBridge(EditorBridgeProps props)
    : editorController(std::move(props.editorController)) {}

bool EditorBridge::isEmpty() const {
  return editorController->buffer_is_empty();
}

QString EditorBridge::getLine(const int index) const {
  return QString::fromUtf8(editorController->get_line(index));
}

QStringList EditorBridge::getLines() const {
  const auto rawLines = editorController->lines();
  QStringList lines = QStringList();

  for (const auto &line : rawLines) {
    lines.append(QString::fromUtf8(line));
  }

  return lines;
}

int EditorBridge::getLineCount() const {
  return static_cast<int>(editorController->get_line_count());
}

Selection EditorBridge::getSelection() {
  const auto selection = editorController->get_selection();
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

std::vector<Cursor> EditorBridge::getCursorPositions() const {
  std::vector<Cursor> cursors;
  const auto nekoCursors = editorController->get_cursor_positions();
  cursors.reserve(nekoCursors.size());

  for (auto cursor : nekoCursors) {
    cursors.push_back(
        {static_cast<int>(cursor.row), static_cast<int>(cursor.col)});
  }
  return cursors;
}

bool EditorBridge::needsWidthMeasurement(const int index) const {
  return editorController->needs_width_measurement(index);
}

double EditorBridge::getMaxWidth() const {
  return editorController->get_max_width();
}

bool EditorBridge::cursorExistsAt(const int row, const int column) const {
  return editorController->cursor_exists_at(row, column);
}

bool EditorBridge::bufferIsEmpty() const {
  return editorController->buffer_is_empty();
}

int EditorBridge::getNumberOfSelections() const {
  return static_cast<int>(editorController->number_of_selections());
}

Cursor EditorBridge::getLastAddedCursor() const {
  const auto nekoCursor = editorController->get_last_added_cursor();
  const auto cursor = Cursor{.row = static_cast<int>(nekoCursor.row),
                             .column = static_cast<int>(nekoCursor.col)};

  return cursor;
}

int EditorBridge::getLineLength(int index) const {
  return static_cast<int>(editorController->line_length(index));
}

void EditorBridge::setLineWidth(const int index, const double width) {
  editorController->update_line_width(index, width);
}

void EditorBridge::setController(
    rust::Box<neko::EditorController> &&controller) {
  this->editorController = std::move(controller);
}

void EditorBridge::selectWord(const int row, const int column) {
  doOp(&neko::EditorController::select_word, row, column);
}

void EditorBridge::selectLine(int row) {
  const int lineCount = static_cast<int>(editorController->get_line_count());
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
        static_cast<int>(editorController->line_length(clampedRow));
    selectTo(clampedRow, lineLength);
  }
}

void EditorBridge::selectWordDrag(const int anchorStartRow,
                                  const int anchorStartColumn,
                                  const int anchorEndRow,
                                  const int anchorEndColumn, const int row,
                                  const int column) {
  doOp(&neko::EditorController::select_word_drag, anchorStartRow,
       anchorStartColumn, anchorEndRow, anchorEndColumn, row, column);
}

void EditorBridge::selectLineDrag(int anchorRow, int row) {
  doOp(&neko::EditorController::select_line_drag, anchorRow, row);
}

void EditorBridge::selectTo(const int row, const int column) {
  doOp(&neko::EditorController::select_to, row, column);
}

void EditorBridge::moveOrSelectLeft(const bool shouldSelect) {
  nav(&neko::EditorController::move_left, &neko::EditorController::select_left,
      shouldSelect);
}

void EditorBridge::moveOrSelectRight(const bool shouldSelect) {
  nav(&neko::EditorController::move_right,
      &neko::EditorController::select_right, shouldSelect);
}

void EditorBridge::moveOrSelectUp(const bool shouldSelect) {
  nav(&neko::EditorController::move_up, &neko::EditorController::select_up,
      shouldSelect);
}

void EditorBridge::moveOrSelectDown(const bool shouldSelect) {
  nav(&neko::EditorController::move_down, &neko::EditorController::select_down,
      shouldSelect);
}

void EditorBridge::moveTo(const int row, const int column,
                          const bool clearSelection) {
  doOp(&neko::EditorController::move_to, row, column, clearSelection);
}

void EditorBridge::insertText(const std::string &text) {
  if (text.empty()) {
    return;
  }

  doOp(&neko::EditorController::insert_text, text);
}

void EditorBridge::insertNewline() {
  doOp(&neko::EditorController::insert_newline);
}

void EditorBridge::insertTab() { doOp(&neko::EditorController::insert_tab); }

void EditorBridge::backspace() { doOp(&neko::EditorController::backspace); }

void EditorBridge::deleteForwards() {
  doOp(&neko::EditorController::delete_forwards);
}

void EditorBridge::selectAll() { doOp(&neko::EditorController::select_all); }

void EditorBridge::copy() { copyToClipboardAndMaybeDelete(false); }

void EditorBridge::cut() { copyToClipboardAndMaybeDelete(true); }

void EditorBridge::paste() {
  const QString text = QApplication::clipboard()->text();
  doOp(&neko::EditorController::paste, text.toStdString());
}

void EditorBridge::undo() { doOp(&neko::EditorController::undo); }

void EditorBridge::redo() { doOp(&neko::EditorController::redo); }

void EditorBridge::clearSelectionOrCursors() {
  if (editorController->has_active_selection()) {
    doOp(&neko::EditorController::clear_selection);
  } else {
    doOp(&neko::EditorController::clear_cursors);
  }
}

// NOLINTNEXTLINE
void EditorBridge::addCursor(neko::AddCursorDirectionKind directionKind,
                             int row, int column) {
  const auto direction =
      EditorBridge::makeCursorDirection(directionKind, row, column);
  editorController->add_cursor(direction);

  emitCursorAndSelection();
  emit viewportChanged();
}

void EditorBridge::removeCursor(int row, int column) {
  editorController->remove_cursor(row, column);

  emitCursorAndSelection();
  emit viewportChanged();
}

void EditorBridge::applyChangeSet(const neko::ChangeSetFfi &changeSet) {
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

void EditorBridge::emitCursorAndSelection() {
  const auto cursors = editorController->get_cursor_positions();
  if (cursors.empty()) {
    return;
  }

  const auto activeIndex = editorController->active_cursor_index();
  const auto &active = cursors[activeIndex];
  const auto [row, col] = normalizeCursorPosition(static_cast<int>(active.row),
                                                  static_cast<int>(active.col));

  const auto cursorCount = static_cast<int>(cursors.size());
  const auto selectionCount =
      static_cast<int>(editorController->number_of_selections());

  emit cursorChanged(row, col, cursorCount, selectionCount);
}

void EditorBridge::emitSelectionOnly() {
  const auto selectionCount =
      static_cast<int>(editorController->number_of_selections());
  emit selectionChanged(selectionCount);
}

void EditorBridge::emitLineCountChanged() {
  const int lineCount = static_cast<int>(editorController->get_line_count());
  emit lineCountChanged(lineCount);
}

void EditorBridge::copyToClipboardAndMaybeDelete(bool deleteAfter) {
  if (!editorController->has_active_selection()) {
    return;
  }

  const auto rawText = editorController->copy();
  if (rawText.empty()) {
    return;
  }

  QApplication::clipboard()->setText(QString::fromUtf8(rawText));

  if (deleteAfter) {
    doOp(&neko::EditorController::delete_forwards);
  }
}

neko::AddCursorDirectionFfi
EditorBridge::makeCursorDirection(neko::AddCursorDirectionKind kind,
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
EditorBridge::normalizeCursorPosition(const int row, const int column) const {
  const int lineCount = static_cast<int>(editorController->get_line_count());
  const int clampedRow = std::clamp(row, 0, std::max(0, lineCount - 1));
  const int lineLength =
      static_cast<int>(editorController->line_length(clampedRow));
  const int clampedCol = std::clamp(column, 0, std::max(0, lineLength));

  return {clampedRow, clampedCol};
}

inline bool EditorBridge::hasFlag(uint32_t mask, uint32_t flag) {
  return (mask & flag) != 0U;
}

template <typename Fn, typename... Args>
void EditorBridge::doOp(Fn &&function, Args &&...args) {
  const auto changeSet =
      std::invoke(std::forward<Fn>(function), editorController,
                  std::forward<Args>(args)...);
  applyChangeSet(changeSet);
}

void EditorBridge::nav(neko::ChangeSetFfi (neko::EditorController::*moveFn)(),
                       neko::ChangeSetFfi (neko::EditorController::*selectFn)(),
                       bool shouldSelect) {
  if (shouldSelect) {
    doOp(selectFn);
  } else {
    doOp(moveFn);
  }
}
