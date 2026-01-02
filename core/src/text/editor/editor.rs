use super::{
    Change, ChangeSet, CursorMode, DeleteResult, Edit, OpFlags, SelectionMode, Transaction,
    UndoHistory, ViewState, WidthManager,
};
use crate::{
    AddCursorDirection, Buffer, Cursor, CursorEntry, CursorManager, Selection, SelectionManager,
};

#[derive(Debug)]
pub struct Editor {
    widths: WidthManager,
    cursor_manager: CursorManager,
    selection_manager: SelectionManager,
    history: UndoHistory,
}

impl Default for Editor {
    fn default() -> Self {
        Self::new()
    }
}

impl Editor {
    pub fn new() -> Self {
        Self {
            widths: WidthManager::new(),
            cursor_manager: CursorManager::new(),
            selection_manager: SelectionManager::new(),
            history: UndoHistory::default(),
        }
    }

    pub fn lines(&self, buffer: &Buffer) -> Vec<String> {
        buffer.get_lines()
    }

    pub fn number_of_selections(&self) -> usize {
        // TODO: When converting to multi-selection, update this
        if self.selection_manager.has_active_selection() {
            1
        } else {
            0
        }
    }

    pub fn line_length(&self, buffer: &Buffer, row: usize) -> usize {
        buffer.line_len(row)
    }

    pub fn last_added_cursor(&self) -> Cursor {
        self.cursor_manager.get_last_added_cursor()
    }

    pub fn active_cursor_index(&self) -> usize {
        let max = self.cursor_manager.cursors.len().saturating_sub(1);
        self.cursor_manager.active_cursor_index.min(max)
    }

    pub(crate) fn selection_manager(&self) -> &SelectionManager {
        &self.selection_manager
    }

    pub(crate) fn selection_manager_mut(&mut self) -> &mut SelectionManager {
        &mut self.selection_manager
    }

    pub(crate) fn cursor_manager(&self) -> &CursorManager {
        &self.cursor_manager
    }

    pub(crate) fn cursor_manager_mut(&mut self) -> &mut CursorManager {
        &mut self.cursor_manager
    }

    pub(crate) fn widths(&self) -> &WidthManager {
        &self.widths
    }

    pub(crate) fn widths_mut(&mut self) -> &mut WidthManager {
        &mut self.widths
    }

    pub fn has_active_selection(&self) -> bool {
        self.selection_manager.has_active_selection()
    }

    pub fn cursor_exists_at(&self, row: usize, col: usize) -> bool {
        self.cursor_manager.cursor_exists_at(row, col)
    }

    pub fn cursor_exists_at_row(&self, row: usize) -> bool {
        self.cursor_manager.cursor_exists_at_row(row)
    }

    pub fn remove_cursor(&mut self, row: usize, col: usize) {
        self.cursor_manager.remove_cursor(row, col);
    }

    pub fn add_cursor(&mut self, buffer: &Buffer, direction: AddCursorDirection) {
        self.cursor_manager.add_cursor(direction, buffer);
    }

    pub fn cursors(&self) -> Vec<CursorEntry> {
        self.cursor_manager.cursors.clone()
    }

    pub fn selection(&self) -> Selection {
        self.selection_manager.selection().clone()
    }

    pub(crate) fn with_op<R>(
        &mut self,
        buffer: &mut Buffer,
        tx: bool,
        flags: OpFlags,
        f: impl FnOnce(&mut Self, &mut Buffer) -> R,
    ) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes(buffer);

        if self.cursor_manager.cursors.len() > 1 {
            self.cursor_manager.sort_and_dedup_cursors();
        }

        if tx {
            self.begin_tx();
        }

        f(self, buffer);

        if tx {
            self.commit_tx();
        }

        if self.cursor_manager.cursors.len() > 1 {
            self.cursor_manager.sort_and_dedup_cursors();
        }

        let mut cs = self.end_changes(buffer, lc0, cur0, &sel0);
        match flags {
            OpFlags::ViewportOnly => cs.change |= Change::VIEWPORT,
            OpFlags::BufferViewportWidths => {
                cs.change |= Change::BUFFER | Change::VIEWPORT | Change::WIDTHS
            }
        }

        cs
    }

    fn begin_changes(&self, buffer: &Buffer) -> (usize, Vec<CursorEntry>, Selection) {
        (
            buffer.line_count(),
            self.cursor_manager.cursors.clone(),
            self.selection_manager.selection().clone(),
        )
    }

    fn end_changes(
        &self,
        buffer: &Buffer,
        before_line_count: usize,
        before_cursors: Vec<CursorEntry>,
        before_selection: &Selection,
    ) -> ChangeSet {
        let after_line_count = buffer.line_count();

        let mut c = Change::NONE;
        if after_line_count != before_line_count {
            c |= Change::LINE_COUNT | Change::VIEWPORT;
        }
        if before_cursors.len() != self.cursor_manager.cursors.len()
            || self
                .cursor_manager
                .cursors
                .iter()
                .zip(before_cursors)
                .any(|(c1, c2)| c1.cursor != c2.cursor)
        {
            c |= Change::CURSOR;
        }
        if self.selection_manager.selection() != before_selection {
            c |= Change::SELECTION;
        }

        ChangeSet {
            change: c,
            line_count_before: before_line_count,
            line_count_after: after_line_count,
            dirty_first_row: None,
            dirty_last_row: None,
        }
    }

    pub fn load_file(&mut self, buffer: &mut Buffer, content: &str) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes(buffer);

        buffer.clear();
        buffer.insert(0, content);

        let cursor_id = self.cursor_manager.new_cursor_id();
        self.cursor_manager
            .set_cursors(vec![CursorEntry::individual(cursor_id, &Cursor::new())]);
        self.selection_manager.reset_selection();

        self.widths
            .clear_and_rebuild_line_widths(buffer.line_count());

        let mut cs = self.end_changes(buffer, lc0, cur0, &sel0);
        cs.change |= Change::BUFFER | Change::VIEWPORT | Change::WIDTHS;
        cs
    }

    pub(crate) fn apply_delete_result(&mut self, buffer: &Buffer, _i: usize, res: DeleteResult) {
        match res {
            DeleteResult::Text { invalidate } => {
                if let Some(line) = invalidate {
                    self.widths.invalidate_line_width(line);
                }
            }
            DeleteResult::Newline {
                row, invalidate, ..
            } => {
                if let Some(line) = invalidate {
                    self.widths.invalidate_line_width(line);
                }
                if row + 1 < buffer.line_count() {
                    self.widths.invalidate_line_width(row + 1);
                }
            }
        }
    }

    pub(crate) fn delete_selection_if_active(&mut self, buffer: &mut Buffer) -> bool {
        if self.selection_manager.selection().is_active() {
            self.delete_selection_impl(buffer);
            true
        } else {
            false
        }
    }

    fn delete_selection_impl(&mut self, buffer: &mut Buffer) {
        let a = self.selection_manager.selection().start.get_idx(buffer);
        let b = self.selection_manager.selection().end.get_idx(buffer);
        let (start, end) = if a <= b { (a, b) } else { (b, a) };

        let cursor_id = self.cursor_manager.new_cursor_id();
        self.cursor_manager
            .clear_and_set_cursors(CursorEntry::individual(
                cursor_id,
                &self.selection_manager.selection.start.clone(),
            ));

        let deleted = buffer.delete_range(start, end);
        self.widths
            .clear_and_rebuild_line_widths(buffer.line_count());
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection_manager.clear_selection();
        self.widths
            .invalidate_line_width(self.cursor_manager.cursors[0].cursor.row);
    }

    pub fn for_each_cursor_rev(&mut self, mut f: impl FnMut(&mut Self, usize)) {
        for i in (0..self.cursor_manager.cursors.len()).rev() {
            f(self, i);
        }
    }

    pub fn move_left(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(
                    buffer,
                    i,
                    SelectionMode::Clear,
                    CursorMode::Multiple,
                    |c, b| c.move_left(b),
                );
            });
        })
    }

    pub fn move_right(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(
                    buffer,
                    i,
                    SelectionMode::Clear,
                    CursorMode::Multiple,
                    |c, b| c.move_right(b),
                );
            });
        })
    }

    pub fn move_up(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(
                    buffer,
                    i,
                    SelectionMode::Clear,
                    CursorMode::Multiple,
                    |c, b| c.move_up(b),
                );
            });
        })
    }

    pub fn move_down(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(
                    buffer,
                    i,
                    SelectionMode::Clear,
                    CursorMode::Multiple,
                    |c, b| c.move_down(b),
                );
            });
        })
    }

    fn move_cursor_at(
        &mut self,
        buffer: &Buffer,
        i: usize,
        mode: SelectionMode,
        cursor_mode: CursorMode,
        f: impl FnOnce(&mut Cursor, &Buffer),
    ) {
        let i = i.min(self.cursor_manager.cursors.len().saturating_sub(1));
        let cursor = self.cursor_manager.cursors[i].clone();

        if let SelectionMode::Extend = mode {
            if !self.selection_manager.selection.is_active() {
                self.selection_manager.ensure_begin(&cursor.cursor);
            }
        }

        let new_i = match cursor_mode {
            CursorMode::Single => {
                self.cursor_manager.clear_and_set_cursors(cursor);
                0
            }
            CursorMode::Multiple => i,
        };

        f(&mut self.cursor_manager.cursors[new_i].cursor, buffer);

        match mode {
            SelectionMode::Extend => self
                .selection_manager
                .selection
                .update(&self.cursor_manager.cursors[new_i].cursor, buffer),
            SelectionMode::Clear => self.selection_manager.clear_selection(),
            SelectionMode::Keep => {}
        }
    }

    pub fn move_to(
        &mut self,
        buffer: &mut Buffer,
        row: usize,
        col: usize,
        clear: bool,
    ) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            let mode = if clear {
                SelectionMode::Clear
            } else {
                SelectionMode::Keep
            };
            editor.move_cursor_at(buffer, i, mode, CursorMode::Single, |c, b| {
                c.move_to(b, row, col)
            });
        })
    }

    pub fn select_word(&mut self, buffer: &mut Buffer, row: usize, column: usize) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.move_cursor_at(
                buffer,
                i,
                SelectionMode::Clear,
                CursorMode::Single,
                |c, b| c.move_to(b, row, column),
            );

            let cursor = editor.cursor_manager.cursors[0].cursor.clone();

            let Some((word_row, word_start, word_end)) =
                editor.word_range_at(buffer, cursor.row, cursor.column)
            else {
                editor.selection_manager.clear_selection();
                return;
            };

            let mut start_cursor = Cursor::new();
            start_cursor.move_to(buffer, word_row, word_start);
            let mut end_cursor = Cursor::new();
            end_cursor.move_to(buffer, word_row, word_end);

            editor.selection_manager.selection.begin(&start_cursor);
            editor
                .selection_manager
                .selection
                .update(&end_cursor, buffer);

            editor.cursor_manager.cursors[0].cursor = end_cursor;
        })
    }

    #[allow(clippy::too_many_arguments)]
    pub fn select_word_drag(
        &mut self,
        buffer: &mut Buffer,
        anchor_start_row: usize,
        anchor_start_col: usize,
        anchor_end_row: usize,
        anchor_end_col: usize,
        row: usize,
        col: usize,
    ) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            let Some((word_row, word_start, word_end)) = editor.word_range_at(buffer, row, col)
            else {
                return;
            };

            let mut anchor_start = Cursor::new();
            anchor_start.move_to(buffer, anchor_start_row, anchor_start_col);

            let mut anchor_end = Cursor::new();
            anchor_end.move_to(buffer, anchor_end_row, anchor_end_col);

            let word_start_pos = (word_row, word_start);
            let word_end_pos = (word_row, word_end);
            let anchor_start_pos = (anchor_start.row, anchor_start.column);
            let anchor_end_pos = (anchor_end.row, anchor_end.column);

            let mut target_start = anchor_start_pos;
            let mut target_end = anchor_end_pos;
            let mut cursor_at_start = false;

            if word_start_pos < anchor_start_pos {
                target_start = word_start_pos;
                cursor_at_start = true;
            } else if word_end_pos > anchor_end_pos {
                target_end = word_end_pos;
            }

            let mut start_cursor = Cursor::new();
            start_cursor.move_to(buffer, target_start.0, target_start.1);

            let mut end_cursor = Cursor::new();
            end_cursor.move_to(buffer, target_end.0, target_end.1);

            editor.selection_manager.selection.begin(&start_cursor);
            editor
                .selection_manager
                .selection
                .update(&end_cursor, buffer);

            if cursor_at_start {
                editor.cursor_manager.cursors[0].cursor = start_cursor;
            } else {
                editor.cursor_manager.cursors[0].cursor = end_cursor;
            }
        })
    }

    pub fn select_line_drag(
        &mut self,
        buffer: &mut Buffer,
        anchor_row: usize,
        row: usize,
    ) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            let line_count = buffer.line_count();
            if line_count == 0 {
                return;
            }

            let anchor_row = anchor_row.min(line_count - 1);
            let row = row.min(line_count - 1);
            let start_row = anchor_row.min(row);
            let end_row = anchor_row.max(row);
            let cursor_at_start = row < anchor_row;

            let mut start_cursor = Cursor::new();
            start_cursor.move_to(buffer, start_row, 0);

            let (end_row, end_col) = if end_row + 1 < line_count {
                (end_row + 1, 0)
            } else {
                (end_row, buffer.line_len(end_row))
            };

            let mut end_cursor = Cursor::new();
            end_cursor.move_to(buffer, end_row, end_col);

            editor.selection_manager.selection.begin(&start_cursor);
            editor
                .selection_manager
                .selection
                .update(&end_cursor, buffer);

            if cursor_at_start {
                editor.cursor_manager.cursors[0].cursor = start_cursor;
            } else {
                editor.cursor_manager.cursors[0].cursor = end_cursor;
            }
        })
    }

    pub fn select_to(&mut self, buffer: &mut Buffer, row: usize, col: usize) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.move_cursor_at(
                buffer,
                i,
                SelectionMode::Extend,
                CursorMode::Multiple,
                |c, b| c.move_to(b, row, col),
            );
        })
    }

    pub fn select_left(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.move_cursor_at(
                buffer,
                i,
                SelectionMode::Extend,
                CursorMode::Multiple,
                |c, b| c.move_left(b),
            );
        })
    }

    pub fn select_right(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.move_cursor_at(
                buffer,
                i,
                SelectionMode::Extend,
                CursorMode::Multiple,
                |c, b| c.move_right(b),
            );
        })
    }

    pub fn select_up(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.move_cursor_at(
                buffer,
                i,
                SelectionMode::Extend,
                CursorMode::Multiple,
                |c, b| c.move_up(b),
            );
        })
    }

    pub fn select_down(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.move_cursor_at(
                buffer,
                i,
                SelectionMode::Extend,
                CursorMode::Multiple,
                |c, b| c.move_down(b),
            );
        })
    }

    pub fn select_all(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, buffer| {
            editor.selection_manager.ensure_begin(&Cursor::new());
            editor.cursor_manager.cursors = vec![CursorEntry::individual(
                editor.cursor_manager.new_cursor_id(),
                &Cursor::new(),
            )];

            editor.cursor_manager.cursors[0].cursor.move_to_end(buffer);
            editor
                .selection_manager
                .selection
                .update(&editor.cursor_manager.cursors[0].cursor, buffer);
        })
    }

    pub fn clear_selection(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, _| {
            editor.selection_manager.clear_selection();
        })
    }

    pub fn clear_cursors(&mut self, buffer: &mut Buffer) -> ChangeSet {
        self.with_op(buffer, false, OpFlags::ViewportOnly, |editor, _| {
            editor.cursor_manager.clear_cursors();
        })
    }

    pub fn needs_width_measurement(&self, line_idx: usize) -> bool {
        self.widths.needs_width_measurement(line_idx)
    }

    pub fn update_line_width(&mut self, line_idx: usize, width: f64) {
        self.widths.update_line_width(line_idx, width);
    }

    fn view_state(&self) -> ViewState {
        ViewState {
            cursors: self.cursor_manager.cursors.clone(),
            selection: self.selection_manager.selection.clone(),
        }
    }

    fn begin_tx(&mut self) {
        if self.history.current.is_none() {
            self.history.current = Some(Transaction {
                before: self.view_state(),
                after: self.view_state(),
                edits: Vec::new(),
            });
        }
    }

    pub(crate) fn record_edit(&mut self, edit: Edit) {
        if let Some(tx) = &mut self.history.current {
            tx.edits.push(edit);
        }
    }

    fn commit_tx(&mut self) {
        if let Some(mut tx) = self.history.current.take() {
            tx.after = self.view_state();

            if !tx.edits.is_empty() {
                self.history.undo.push(tx);
                self.history.redo.clear();
            }
        }
    }

    pub fn undo(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes(buffer);

        self.history.current = None;

        let Some(tx) = self.history.undo.pop() else {
            return ChangeSet::default();
        };

        for edit in tx.edits.iter().rev() {
            edit.invert().apply(buffer);
        }

        self.cursor_manager.set_cursors(tx.before.cursors.clone());
        self.selection_manager.set_selection(&tx.before.selection);

        self.widths
            .clear_and_rebuild_line_widths(buffer.line_count());

        self.history.redo.push(tx);

        let mut cs = self.end_changes(buffer, lc0, cur0, &sel0);
        cs.change |= Change::SELECTION
            | Change::BUFFER
            | Change::CURSOR
            | Change::VIEWPORT
            | Change::WIDTHS
            | Change::LINE_COUNT;
        cs
    }

    pub fn redo(&mut self, buffer: &mut Buffer) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes(buffer);

        self.history.current = None;

        let Some(tx) = self.history.redo.pop() else {
            return ChangeSet::default();
        };

        for edit in tx.edits.iter() {
            edit.apply(buffer);
        }

        self.cursor_manager.set_cursors(tx.after.cursors.clone());
        self.selection_manager.set_selection(&tx.after.selection);

        self.widths
            .clear_and_rebuild_line_widths(buffer.line_count());

        self.history.undo.push(tx);

        let mut cs = self.end_changes(buffer, lc0, cur0, &sel0);
        cs.change |= Change::SELECTION
            | Change::BUFFER
            | Change::CURSOR
            | Change::VIEWPORT
            | Change::WIDTHS
            | Change::LINE_COUNT;
        cs
    }

    fn word_range_at(
        &self,
        buffer: &Buffer,
        row: usize,
        column: usize,
    ) -> Option<(usize, usize, usize)> {
        let line_len = buffer.line_len_without_newline(row);

        if line_len == 0 {
            return None;
        }

        let line = buffer.get_line(row);
        let line_bytes = line.as_bytes();
        let line_len = line_len.min(line_bytes.len());

        if line_len == 0 {
            return None;
        }

        let target_col = if column >= line_len {
            line_len.saturating_sub(1)
        } else {
            column
        };

        let classify = |b: u8| {
            if b.is_ascii_alphanumeric() || b == b'_' {
                0u8
            } else if b.is_ascii_whitespace() {
                1u8
            } else {
                2u8
            }
        };

        let kind = classify(line_bytes[target_col]);

        let mut start = target_col;
        while start > 0 && classify(line_bytes[start - 1]) == kind {
            start -= 1;
        }

        let mut end = target_col;
        while end + 1 < line_len && classify(line_bytes[end + 1]) == kind {
            end += 1;
        }

        Some((row, start, end + 1))
    }

    pub fn buffer_is_empty(&self, buffer: &Buffer) -> bool {
        buffer.is_empty()
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn select_word_highlights_word() {
        let mut editor = Editor::new();
        let mut buffer = Buffer::new();

        editor.load_file(&mut buffer, "hello world");
        editor.select_word(&mut buffer, 0, 1);

        let selection = editor.selection_manager.selection.clone();
        assert!(selection.is_active());
        assert_eq!((selection.start.row, selection.start.column), (0, 0));
        assert_eq!((selection.end.row, selection.end.column), (0, 5));
        assert_eq!(
            (
                editor.cursor_manager.cursors[0].cursor.row,
                editor.cursor_manager.cursors[0].cursor.column
            ),
            (0, 5)
        );
    }

    #[test]
    fn select_word_from_line_end_selects_previous_word() {
        let mut editor = Editor::new();
        let mut buffer = Buffer::new();

        editor.load_file(&mut buffer, "hello world");
        editor.select_word(&mut buffer, 0, 11);

        let selection = editor.selection_manager.selection.clone();
        assert!(selection.is_active());
        assert_eq!((selection.start.row, selection.start.column), (0, 6));
        assert_eq!((selection.end.row, selection.end.column), (0, 11));
        assert_eq!(
            (
                editor.cursor_manager.cursors[0].cursor.row,
                editor.cursor_manager.cursors[0].cursor.column
            ),
            (0, 11)
        );
    }

    #[test]
    fn insert_text_replaces_selection_at_anchor() {
        let mut editor = Editor::new();
        let mut buffer = Buffer::new();

        editor.load_file(&mut buffer, "hello world");
        editor.move_to(&mut buffer, 0, 6, true);
        editor.select_to(&mut buffer, 0, 11);
        editor.insert_text(&mut buffer, "cat");

        assert_eq!(buffer.get_text(), "hello cat");
        assert_eq!(editor.cursor_manager.cursors.len(), 1);
        assert_eq!(
            (
                editor.cursor_manager.cursors[0].cursor.row,
                editor.cursor_manager.cursors[0].cursor.column
            ),
            (0, 9)
        );
    }

    #[test]
    fn insert_text_after_select_all_keeps_buffer_valid() {
        let mut editor = Editor::new();
        let mut buffer = Buffer::new();

        editor.load_file(&mut buffer, "abc");
        editor.select_all(&mut buffer);
        editor.insert_text(&mut buffer, "z");

        assert_eq!(buffer.get_text(), "z");
        assert_eq!(
            (
                editor.cursor_manager.cursors[0].cursor.row,
                editor.cursor_manager.cursors[0].cursor.column
            ),
            (0, 1)
        );
    }

    #[test]
    fn inserting_after_selection_keeps_other_cursors() {
        let mut editor = Editor::new();
        let mut buffer = Buffer::new();

        editor.load_file(&mut buffer, "abcd\nefgh");

        // Active cursor selects "bc"
        editor.move_to(&mut buffer, 0, 1, true);
        // Add another cursor on the second line
        editor
            .cursor_manager
            .add_cursor(AddCursorDirection::At { row: 1, col: 2 }, &buffer);
        editor.select_to(&mut buffer, 0, 3);

        editor.insert_text(&mut buffer, "X");

        let positions: Vec<(usize, usize)> = editor
            .cursor_manager
            .cursors
            .iter()
            .map(|c| (c.cursor.row, c.cursor.column))
            .collect();

        assert_eq!(buffer.get_text(), "aXd\nefXgh");
        assert_eq!(editor.cursor_manager.cursors.len(), 2);
        assert!(positions.contains(&(0, 2)));
        assert!(positions.contains(&(1, 3)));
    }
}
