use std::fmt::Debug;

use crate::{Buffer, Cursor, Selection};

use super::{
    Change, ChangeSet, Edit, Transaction, UndoHistory, ViewState,
    change_set::OpFlags,
    cursor_manager::{AddCursorDirection, CursorEntry, CursorManager},
};

enum SelectionMode {
    Clear,
    Extend,
    Keep,
}

enum CursorMode {
    Single,
    Multiple,
}

enum DeleteResult {
    Text {
        invalidate: Option<usize>,
    },
    Newline {
        row: usize,
        invalidate: Option<usize>,
    },
}

#[derive(Default, Debug)]
pub struct Editor {
    pub(crate) buffer: Buffer,
    line_widths: Vec<f64>,
    pub(crate) max_width: f64,
    max_width_line: usize,
    cursor_manager: CursorManager,
    // Single shared selection across all cursors; TODO multi-selection per cursor
    pub(crate) selection: Selection,
    history: UndoHistory,
}

impl Editor {
    pub fn new() -> Self {
        Self {
            buffer: Buffer::new(),
            line_widths: vec![-1.0],
            max_width: 0.0,
            max_width_line: 0,
            cursor_manager: CursorManager::new(),
            selection: Selection::new(),
            history: UndoHistory::default(),
        }
    }

    pub fn has_active_selection(&self) -> bool {
        self.selection.is_active()
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

    pub fn add_cursor(&mut self, direction: AddCursorDirection) {
        self.cursor_manager.add_cursor(direction, &self.buffer);
    }

    pub fn cursors(&self) -> Vec<CursorEntry> {
        self.cursor_manager.cursors.clone()
    }

    fn with_op<R>(
        &mut self,
        tx: bool,
        flags: OpFlags,
        f: impl FnOnce(&mut Self) -> R,
    ) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes();

        if self.cursor_manager.cursors.len() > 1 {
            self.cursor_manager.sort_and_dedup_cursors();
        }

        if tx {
            self.begin_tx();
        }

        f(self);

        if tx {
            self.commit_tx();
        }

        if self.cursor_manager.cursors.len() > 1 {
            self.cursor_manager.sort_and_dedup_cursors();
        }

        let mut cs = self.end_changes(lc0, cur0, &sel0);
        match flags {
            OpFlags::ViewportOnly => cs.change |= Change::VIEWPORT,
            OpFlags::BufferViewportWidths => {
                cs.change |= Change::BUFFER | Change::VIEWPORT | Change::WIDTHS
            }
        }

        cs
    }

    fn begin_changes(&self) -> (usize, Vec<CursorEntry>, Selection) {
        (
            self.buffer.line_count(),
            self.cursor_manager.cursors.clone(),
            self.selection.clone(),
        )
    }

    fn end_changes(
        &self,
        before_line_count: usize,
        before_cursors: Vec<CursorEntry>,
        before_selection: &Selection,
    ) -> ChangeSet {
        let after_line_count = self.buffer.line_count();

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
        if &self.selection != before_selection {
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

    pub fn load_file(&mut self, content: &str) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes();

        self.buffer.clear();
        self.buffer.insert(0, content);

        let cursor_id = self.cursor_manager.new_cursor_id();
        self.cursor_manager
            .set_cursors(vec![CursorEntry::individual(cursor_id, &Cursor::new())]);
        self.selection = Selection::new();

        self.clear_and_rebuild_line_widths();

        let mut cs = self.end_changes(lc0, cur0, &sel0);
        cs.change |= Change::BUFFER | Change::VIEWPORT | Change::WIDTHS;
        cs
    }

    fn clear_and_rebuild_line_widths(&mut self) {
        // Resize line_widths to match buffer
        let line_count = self.buffer.line_count();
        self.line_widths.clear();
        self.line_widths.resize(line_count, -1.0);
        self.max_width = 0.0;
        self.max_width_line = 0;
    }

    fn apply_delete_result(&mut self, _i: usize, res: DeleteResult) {
        match res {
            DeleteResult::Text { invalidate } => {
                if let Some(line) = invalidate {
                    self.invalidate_line_width(line);
                }
            }
            DeleteResult::Newline {
                row, invalidate, ..
            } => {
                if let Some(line) = invalidate {
                    self.invalidate_line_width(line);
                }
                if row + 1 < self.buffer.line_count() {
                    self.invalidate_line_width(row + 1);
                }
            }
        }
    }

    fn delete_selection_if_active(&mut self) -> bool {
        if self.selection.is_active() {
            self.delete_selection_impl();
            true
        } else {
            false
        }
    }

    fn delete_selection_impl(&mut self) {
        let a = self.selection.start.get_idx(&self.buffer);
        let b = self.selection.end.get_idx(&self.buffer);
        let (start, end) = if a <= b { (a, b) } else { (b, a) };

        let cursor_id = self.cursor_manager.new_cursor_id();
        self.cursor_manager
            .set_cursors(vec![CursorEntry::individual(
                cursor_id,
                &self.selection.start.clone(),
            )]);

        let deleted = self.buffer.delete_range(start, end);
        self.clear_and_rebuild_line_widths();
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection.clear();
        self.invalidate_line_width(self.cursor_manager.cursors[0].cursor.row);
    }

    pub fn insert_text(&mut self, text: &str) -> ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            let has_nl = text.contains('\n');

            let mut idxs: Vec<usize> = editor
                .delete_selection_preserve_cursors()
                .unwrap_or_else(|| editor.cursor_manager.cursor_byte_idxs(&editor.buffer));

            editor.for_each_cursor_rev(|editor, i| {
                editor.insert_at(text, i, &mut idxs, |pos: usize, editor: &mut Editor| {
                    editor.invalidate_insert_lines(pos, text, has_nl);
                });
            });

            for (c, &idx) in editor.cursor_manager.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.cursor.move_to(&editor.buffer, row, col);
            }
        });

        self.sync_line_widths();
        res
    }

    fn delete_selection_preserve_cursors(&mut self) -> Option<Vec<usize>> {
        if !self.selection.is_active() {
            return None;
        }

        let a = self.selection.start.get_idx(&self.buffer);
        let b = self.selection.end.get_idx(&self.buffer);
        let (start, end) = if a <= b { (a, b) } else { (b, a) };
        let delta = end - start;

        let cursor_info: Vec<(CursorEntry, usize)> = self
            .cursor_manager
            .cursors
            .iter()
            .cloned()
            .map(|entry| {
                let idx = entry.cursor.get_idx(&self.buffer);
                (entry, idx)
            })
            .collect();
        let active_id = self
            .cursor_manager
            .cursors
            .get(self.cursor_manager.active_cursor_index)
            .map(|c| c.id);

        let deleted = self.buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection.clear();
        self.clear_and_rebuild_line_widths();

        let mut new_cursors: Vec<CursorEntry> = Vec::new();
        for (entry, idx) in cursor_info {
            let new_idx = if idx <= start {
                idx
            } else if idx >= end {
                idx - delta
            } else {
                continue;
            };

            let (row, col) = self.buffer.byte_to_row_col(new_idx);
            let mut cursor = entry.cursor.clone();
            cursor.move_to(&self.buffer, row, col);
            new_cursors.push(CursorEntry {
                id: entry.id,
                cursor,
                column_group: entry.column_group,
            });
        }

        if new_cursors.is_empty() {
            let mut cursor = Cursor::new();
            let (row, col) = self.buffer.byte_to_row_col(start);
            cursor.move_to(&self.buffer, row, col);
            new_cursors.push(CursorEntry::individual(
                self.cursor_manager.new_cursor_id(),
                &cursor,
            ));
        }

        self.cursor_manager.set_cursors(new_cursors);
        self.cursor_manager.sort_and_dedup_cursors();

        if let Some(id) = active_id {
            self.cursor_manager.set_active_cursor_index(
                self.cursor_manager
                    .cursors
                    .iter()
                    .position(|c| c.id == id)
                    .unwrap_or(0),
            );
        } else {
            self.cursor_manager.set_active_cursor_index(0);
        }

        Some(self.cursor_manager.cursor_byte_idxs(&self.buffer))
    }

    fn insert_at(
        &mut self,
        text: &str,
        i: usize,
        idxs: &mut [usize],
        invalidate_lines: impl FnOnce(usize, &mut Editor),
    ) {
        let pos = idxs[i];

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let ins_len = text.len();

        (0..idxs.len()).for_each(|j| {
            if idxs[j] > pos {
                idxs[j] += ins_len;
            } else if idxs[j] == pos {
                idxs[j] = pos + ins_len;
            }
        });

        invalidate_lines(pos, self);
    }

    fn invalidate_insert_lines(&mut self, pos: usize, text: &str, has_nl: bool) {
        let row = self.buffer.byte_to_line(pos);
        self.invalidate_line_width(row);

        if has_nl {
            let extra_lines = text.bytes().filter(|b| *b == b'\n').count();
            for offset in 1..=extra_lines {
                let line = row + offset;
                if line < self.buffer.line_count() {
                    self.invalidate_line_width(line);
                }
            }
        }
    }

    pub fn for_each_cursor_rev(&mut self, mut f: impl FnMut(&mut Self, usize)) {
        for i in (0..self.cursor_manager.cursors.len()).rev() {
            f(self, i);
        }
    }

    pub fn backspace(&mut self) -> ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            if editor.delete_selection_if_active() {
                return;
            }

            let mut idxs: Vec<usize> = editor
                .cursor_manager
                .cursors
                .iter()
                .map(|c| c.cursor.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                let res = editor.backspace_impl(i, &mut idxs);
                editor.apply_delete_result(i, res);
            });

            for (c, &idx) in editor.cursor_manager.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.cursor.move_to(&editor.buffer, row, col);
            }
        });

        self.sync_line_widths();
        res
    }

    fn backspace_impl(&mut self, i: usize, idxs: &mut [usize]) -> DeleteResult {
        let pos = idxs[i];
        if pos == 0 {
            return DeleteResult::Text { invalidate: None };
        }

        let start = if pos >= 2 && self.buffer.get_text_range(pos - 2, pos) == "\r\n" {
            pos - 2
        } else {
            pos - 1
        };
        let end = pos;

        let deleted = self.buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted: deleted.clone(),
        });

        self.apply_delete_to_idxs(start, end, idxs, i);

        if deleted.contains('\n') {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Newline {
                row,
                invalidate: Some(row),
            }
        } else {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Text {
                invalidate: Some(row),
            }
        }
    }

    fn apply_delete_to_idxs(&self, start: usize, end: usize, idxs: &mut [usize], caret_i: usize) {
        let delta = end - start;

        (0..idxs.len()).for_each(|j| {
            let idx = idxs[j];
            if idx > end {
                idxs[j] = idx - delta;
            } else if idx > start {
                idxs[j] = start;
            }
        });

        idxs[caret_i] = start;
    }

    pub fn delete(&mut self) -> ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            if editor.delete_selection_if_active() {
                return;
            }

            let mut idxs: Vec<usize> = editor
                .cursor_manager
                .cursors
                .iter()
                .map(|c| c.cursor.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                let res = editor.delete_impl(i, &mut idxs);
                editor.apply_delete_result(i, res);
            });

            for (c, &idx) in editor.cursor_manager.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.cursor.move_to(&editor.buffer, row, col);
            }
        });

        self.sync_line_widths();
        res
    }

    fn delete_impl(&mut self, i: usize, idxs: &mut [usize]) -> DeleteResult {
        let start = idxs[i];
        if start >= self.buffer.byte_len().saturating_sub(1) {
            return DeleteResult::Text { invalidate: None };
        }

        let mut end = start + 1;
        if self.buffer.get_text_range(start, end) == "\r"
            && start + 2 <= self.buffer.byte_len()
            && self.buffer.get_text_range(start, start + 2) == "\r\n"
        {
            end = start + 2;
        }

        let deleted = self.buffer.delete_range(start, end);
        if deleted.is_empty() {
            return DeleteResult::Text { invalidate: None };
        }

        self.record_edit(Edit::Delete {
            start,
            end,
            deleted: deleted.clone(),
        });

        self.apply_delete_to_idxs(start, end, idxs, i);

        if deleted.contains('\n') {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Newline {
                row,
                invalidate: Some(row),
            }
        } else {
            let row = self.buffer.byte_to_line(start);
            DeleteResult::Text {
                invalidate: Some(row),
            }
        }
    }

    pub fn move_left(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_left(b)
                });
            });
        })
    }

    pub fn move_right(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_right(b)
                });
            });
        })
    }

    pub fn move_up(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_up(b)
                });
            });
        })
    }

    pub fn move_down(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.for_each_cursor_rev(|editor, i| {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_down(b)
                });
            });
        })
    }

    fn move_cursor_at(
        &mut self,
        i: usize,
        mode: SelectionMode,
        cursor_mode: CursorMode,
        f: impl FnOnce(&mut Cursor, &Buffer),
    ) {
        let i = i.min(self.cursor_manager.cursors.len().saturating_sub(1));
        let cursor = self.cursor_manager.cursors[i].clone();

        if let SelectionMode::Extend = mode {
            if !self.selection.is_active() {
                self.selection.begin(&cursor.cursor);
            }
        }

        let new_i = match cursor_mode {
            CursorMode::Single => {
                self.cursor_manager.clear_and_set_cursors(cursor);
                0
            }
            CursorMode::Multiple => i,
        };

        f(&mut self.cursor_manager.cursors[new_i].cursor, &self.buffer);

        match mode {
            SelectionMode::Extend => self
                .selection
                .update(&self.cursor_manager.cursors[new_i].cursor, &self.buffer),
            SelectionMode::Clear => self.selection.clear(),
            SelectionMode::Keep => {}
        }
    }

    pub fn move_to(&mut self, row: usize, col: usize, clear: bool) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            let mode = if clear {
                SelectionMode::Clear
            } else {
                SelectionMode::Keep
            };
            editor.move_cursor_at(i, mode, CursorMode::Single, |c, b| c.move_to(b, row, col));
        })
    }

    pub fn select_to(&mut self, row: usize, col: usize) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Multiple, |c, b| {
                c.move_to(b, row, col)
            });
        })
    }

    pub fn select_left(&mut self) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Multiple, |c, b| {
                c.move_left(b)
            });
        })
    }

    pub fn select_right(&mut self) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Multiple, |c, b| {
                c.move_right(b)
            });
        })
    }

    pub fn select_up(&mut self) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Multiple, |c, b| {
                c.move_up(b)
            });
        })
    }

    pub fn select_down(&mut self) -> ChangeSet {
        let i = self.cursor_manager.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Multiple, |c, b| {
                c.move_down(b)
            });
        })
    }

    pub fn select_all(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.selection.begin(&Cursor::new());
            editor.cursor_manager.cursors = vec![CursorEntry::individual(
                editor.cursor_manager.new_cursor_id(),
                &Cursor::new(),
            )];

            editor.cursor_manager.cursors[0]
                .cursor
                .move_to_end(&editor.buffer);
            editor
                .selection
                .update(&editor.cursor_manager.cursors[0].cursor, &editor.buffer);
        })
    }

    pub fn clear_selection(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.selection.clear()
        })
    }

    pub fn clear_cursors(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.cursor_manager.clear_cursors();
        })
    }

    fn sync_line_widths(&mut self) {
        let line_count = self.buffer.line_count();

        match line_count.cmp(&self.line_widths.len()) {
            std::cmp::Ordering::Greater => {
                // Buffer grew, add invalidated entries
                self.line_widths.resize(line_count, -1.0);
            }
            std::cmp::Ordering::Less => {
                // Buffer shrunk, remove entries
                self.line_widths.truncate(line_count);

                // If we removed the max width line, recalculate
                if self.max_width_line >= line_count {
                    self.recalculate_max_width();
                }
            }
            std::cmp::Ordering::Equal => {}
        }
    }

    pub fn invalidate_line_width(&mut self, line_idx: usize) {
        if line_idx >= self.line_widths.len() {
            return;
        }

        self.line_widths[line_idx] = -1.0;

        if line_idx == self.max_width_line {
            self.recalculate_max_width();
        }
    }

    pub fn needs_width_measurement(&self, line_idx: usize) -> bool {
        if line_idx >= self.line_widths.len() {
            return false;
        }

        self.line_widths[line_idx] == -1.0
    }

    pub fn update_line_width(&mut self, line_idx: usize, width: f64) {
        if line_idx >= self.line_widths.len() {
            return;
        }

        self.line_widths[line_idx] = width;

        if width > self.max_width {
            self.max_width = width;
            self.max_width_line = line_idx;
        }
    }

    fn recalculate_max_width(&mut self) {
        self.max_width = 0.0;
        self.max_width_line = 0;

        for (idx, &width) in self.line_widths.iter().enumerate() {
            if width > self.max_width {
                self.max_width = width;
                self.max_width_line = idx;
            }
        }
    }

    fn view_state(&self) -> ViewState {
        ViewState {
            cursors: self.cursor_manager.cursors.clone(),
            selection: self.selection.clone(),
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

    fn record_edit(&mut self, edit: Edit) {
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

    pub fn undo(&mut self) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes();

        self.history.current = None;

        let Some(tx) = self.history.undo.pop() else {
            return ChangeSet::default();
        };

        for edit in tx.edits.iter().rev() {
            edit.invert().apply(&mut self.buffer);
        }

        self.cursor_manager.set_cursors(tx.before.cursors.clone());
        self.selection = tx.before.selection.clone();

        self.clear_and_rebuild_line_widths();

        self.history.redo.push(tx);

        let mut cs = self.end_changes(lc0, cur0, &sel0);
        cs.change |= Change::SELECTION
            | Change::BUFFER
            | Change::CURSOR
            | Change::VIEWPORT
            | Change::WIDTHS
            | Change::LINE_COUNT;
        cs
    }

    pub fn redo(&mut self) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes();

        self.history.current = None;

        let Some(tx) = self.history.redo.pop() else {
            return ChangeSet::default();
        };

        for edit in tx.edits.iter() {
            edit.apply(&mut self.buffer);
        }

        self.cursor_manager.set_cursors(tx.after.cursors.clone());
        self.selection = tx.after.selection.clone();

        self.clear_and_rebuild_line_widths();

        self.history.undo.push(tx);

        let mut cs = self.end_changes(lc0, cur0, &sel0);
        cs.change |= Change::SELECTION
            | Change::BUFFER
            | Change::CURSOR
            | Change::VIEWPORT
            | Change::WIDTHS
            | Change::LINE_COUNT;
        cs
    }
}

#[cfg(test)]
mod tests {
    use crate::text::cursor_manager::AddCursorDirection;

    use super::*;

    #[test]
    fn insert_text_replaces_selection_at_anchor() {
        let mut editor = Editor::new();
        editor.load_file("hello world");
        editor.move_to(0, 6, true);
        editor.select_to(0, 11);

        editor.insert_text("cat");

        assert_eq!(editor.buffer.get_text(), "hello cat");
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
        editor.load_file("abc");
        editor.select_all();

        editor.insert_text("z");

        assert_eq!(editor.buffer.get_text(), "z");
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
        editor.load_file("abcd\nefgh");

        // Active cursor selects "bc"
        editor.move_to(0, 1, true);
        // Add another cursor on the second line
        editor
            .cursor_manager
            .add_cursor(AddCursorDirection::At { row: 1, col: 2 }, &editor.buffer);
        editor.select_to(0, 3);

        editor.insert_text("X");

        let positions: Vec<(usize, usize)> = editor
            .cursor_manager
            .cursors
            .iter()
            .map(|c| (c.cursor.row, c.cursor.column))
            .collect();

        assert_eq!(editor.buffer.get_text(), "aXd\nefXgh");
        assert_eq!(editor.cursor_manager.cursors.len(), 2);
        assert!(positions.contains(&(0, 2)));
        assert!(positions.contains(&(1, 3)));
    }
}
