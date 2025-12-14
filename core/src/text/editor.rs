use std::fmt::Debug;

use crate::{Buffer, Cursor, Selection};

use super::{Change, ChangeSet, Edit, Transaction, UndoHistory, ViewState, change_set::OpFlags};

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

pub enum AddCursorDirection {
    Above,
    Below,
    At { row: usize, col: usize },
}

#[derive(Default, Debug)]
pub struct Editor {
    pub(crate) buffer: Buffer,
    line_widths: Vec<f64>,
    pub(crate) max_width: f64,
    max_width_line: usize,
    pub(crate) cursors: Vec<Cursor>,
    pub(crate) active_cursor_index: usize,
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
            cursors: vec![Cursor::new()],
            active_cursor_index: 0,
            selection: Selection::new(),
            history: UndoHistory::default(),
        }
    }

    pub fn cursor_exists_at(&self, row: usize, col: usize) -> bool {
        self.cursors.iter().any(|c| c.row == row && c.column == col)
    }

    fn cursor_exists_at_row(&self, row: usize) -> bool {
        self.cursors.iter().any(|c| c.row == row)
    }

    pub fn remove_cursor(&mut self, row: usize, col: usize) {
        if self.cursors.len() == 1 {
            return;
        }

        let index = self
            .cursors
            .iter()
            .position(|c| c.row == row && c.column == col);

        if let Some(found_index) = index {
            self.cursors.remove(found_index);

            if found_index == self.active_cursor_index
                || self.active_cursor_index >= self.cursors.len()
            {
                self.active_cursor_index = self.cursors.len() - 1;
            }
        }
    }

    pub fn clear_cursors(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            let active_cursor = editor.cursors[editor.active_cursor_index].clone();
            editor.cursors.clear();
            editor.cursors.push(active_cursor);
            editor.active_cursor_index = 0;
        })
    }

    pub fn has_active_selection(&self) -> bool {
        self.selection.is_active()
    }

    fn sort_and_dedup_cursors(&mut self) {
        let active_key = {
            let c = &self.cursors[self.active_cursor_index];
            (c.row, c.column)
        };

        self.cursors.sort_by_key(|c| (c.row, c.column));
        self.cursors.dedup_by_key(|c| (c.row, c.column));

        self.active_cursor_index = self
            .cursors
            .iter()
            .position(|c| (c.row, c.column) == active_key)
            .unwrap_or(0);
    }

    fn with_op<R>(
        &mut self,
        tx: bool,
        flags: OpFlags,
        f: impl FnOnce(&mut Self) -> R,
    ) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes();

        self.sort_and_dedup_cursors();

        if tx {
            self.begin_tx();
        }

        f(self);

        if tx {
            self.commit_tx();
        }

        self.sort_and_dedup_cursors();

        let mut cs = self.end_changes(lc0, cur0, &sel0);
        match flags {
            OpFlags::ViewportOnly => cs.change |= Change::VIEWPORT,
            OpFlags::BufferViewportWidths => {
                cs.change |= Change::BUFFER | Change::VIEWPORT | Change::WIDTHS
            }
        }

        cs
    }

    fn begin_changes(&self) -> (usize, Vec<Cursor>, Selection) {
        (
            self.buffer.line_count(),
            self.cursors.clone(),
            self.selection.clone(),
        )
    }

    fn end_changes(
        &self,
        before_line_count: usize,
        before_cursors: Vec<Cursor>,
        before_selection: &Selection,
    ) -> ChangeSet {
        let after_line_count = self.buffer.line_count();

        let mut c = Change::NONE;
        if after_line_count != before_line_count {
            c |= Change::LINE_COUNT | Change::VIEWPORT;
        }
        if before_cursors.len() != self.cursors.len()
            || self
                .cursors
                .iter()
                .zip(before_cursors)
                .any(|(c1, c2)| *c1 != c2)
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
        self.cursors = vec![Cursor::new()];
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

    fn for_each_cursor_rev(&mut self, mut f: impl FnMut(&mut Self, usize)) {
        for i in (0..self.cursors.len()).rev() {
            f(self, i);
        }
    }

    fn delete_selection_impl(&mut self) {
        let a = self.selection.start.get_idx(&self.buffer);
        let b = self.selection.end.get_idx(&self.buffer);
        let (start, end) = if a <= b { (a, b) } else { (b, a) };

        self.cursors = vec![self.selection.start.clone()];

        let deleted = self.buffer.delete_range(start, end);
        self.clear_and_rebuild_line_widths();
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection.clear();
        self.invalidate_line_width(self.cursors[0].row);
    }

    pub fn insert_text(&mut self, text: &str) -> ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            let has_nl = text.contains('\n');
            let is_tab = text == "\t";

            let mut idxs: Vec<usize> = editor
                .cursors
                .iter()
                .map(|c| c.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                if has_nl {
                    editor.insert_newline(text, i, &mut idxs);
                } else if is_tab {
                    editor.insert_tab(i, &mut idxs);
                } else {
                    editor.insert_text_internal(text, i, &mut idxs);
                }
            });

            for (c, &idx) in editor.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.move_to(&editor.buffer, row, col);
            }
        });

        self.sync_line_widths();
        res
    }

    fn insert_text_internal(&mut self, text: &str, i: usize, idxs: &mut [usize]) {
        self.delete_selection_if_active();

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

        let row = self.buffer.byte_to_line(pos);
        self.invalidate_line_width(row);
    }

    fn insert_newline(&mut self, text: &str, i: usize, idxs: &mut [usize]) {
        self.delete_selection_if_active();

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

        let row = self.buffer.byte_to_line(pos);
        self.invalidate_line_width(row);
        if row + 1 < self.buffer.line_count() {
            self.invalidate_line_width(row + 1);
        }
    }

    fn insert_tab(&mut self, i: usize, idxs: &mut [usize]) {
        self.delete_selection_if_active();

        let text = "    ";
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

        let row = self.buffer.byte_to_line(pos);
        self.invalidate_line_width(row);
    }

    pub fn backspace(&mut self) -> ChangeSet {
        let res = self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            if editor.delete_selection_if_active() {
                return;
            }

            let mut idxs: Vec<usize> = editor
                .cursors
                .iter()
                .map(|c| c.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                let res = editor.backspace_impl(i, &mut idxs);
                editor.apply_delete_result(i, res);
            });

            for (c, &idx) in editor.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.move_to(&editor.buffer, row, col);
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
                .cursors
                .iter()
                .map(|c| c.get_idx(&editor.buffer))
                .collect();

            editor.for_each_cursor_rev(|editor, i| {
                let res = editor.delete_impl(i, &mut idxs);
                editor.apply_delete_result(i, res);
            });

            for (c, &idx) in editor.cursors.iter_mut().zip(idxs.iter()) {
                let (row, col) = editor.buffer.byte_to_row_col(idx);
                c.move_to(&editor.buffer, row, col);
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

    fn move_cursor_at(
        &mut self,
        i: usize,
        mode: SelectionMode,
        cursor_mode: CursorMode,
        f: impl FnOnce(&mut Cursor, &Buffer),
    ) {
        self.sort_and_dedup_cursors();

        let i = i.min(self.cursors.len().saturating_sub(1));
        let cursor = self.cursors[i].clone();

        if let SelectionMode::Extend = mode {
            if !self.selection.is_active() {
                self.selection.begin(&cursor);
            }
        }

        let new_i = match cursor_mode {
            CursorMode::Single => {
                self.cursors.clear();
                self.cursors.push(cursor);
                self.active_cursor_index = 0;
                0
            }
            CursorMode::Multiple => i,
        };

        f(&mut self.cursors[new_i], &self.buffer);

        self.sort_and_dedup_cursors();

        match mode {
            SelectionMode::Extend => self.selection.update(&self.cursors[new_i], &self.buffer),
            SelectionMode::Clear => self.selection.clear(),
            SelectionMode::Keep => {}
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

    pub fn select_left(&mut self) -> ChangeSet {
        let i = self.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                c.move_left(b)
            });
        })
    }

    pub fn select_right(&mut self) -> ChangeSet {
        let i = self.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                c.move_right(b)
            });
        })
    }

    pub fn select_up(&mut self) -> ChangeSet {
        let i = self.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                c.move_up(b)
            });
        })
    }

    pub fn select_down(&mut self) -> ChangeSet {
        let i = self.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                c.move_down(b)
            });
        })
    }

    pub fn move_to(&mut self, row: usize, col: usize, clear: bool) -> ChangeSet {
        let cursor_index = self.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            let mode = if clear {
                SelectionMode::Clear
            } else {
                SelectionMode::Keep
            };
            editor.move_cursor_at(cursor_index, mode, CursorMode::Single, |c, b| {
                c.move_to(b, row, col)
            });
        })
    }

    fn cursors_are_contiguous(&self, first_cursor: &Cursor, last_cursor: &Cursor) -> bool {
        let starting_row = first_cursor.row;
        let ending_row = last_cursor.row;

        for i in starting_row..ending_row {
            if !self.cursor_exists_at_row(i) {
                return false;
            }
        }

        true
    }

    pub fn add_cursor(&mut self, direction: AddCursorDirection) {
        match direction {
            AddCursorDirection::Above => self.add_cursor_above(),
            AddCursorDirection::Below => self.add_cursor_below(),
            AddCursorDirection::At { row, col } => self.add_cursor_at(row, col),
        }
    }

    fn add_cursor_above(&mut self) {
        let main_cursor = self.cursors[self.active_cursor_index].clone();
        let last_cursor = self.cursors.last().unwrap_or(&main_cursor);

        // If cursors have been added below, reverse them first
        if self.cursors_are_contiguous(&main_cursor, last_cursor)
            && last_cursor.row > main_cursor.row
        {
            self.remove_cursor(last_cursor.row, last_cursor.column);
        } else {
            self.for_each_cursor_rev(|editor, i| {
                let (row, col) = (editor.cursors[i].row, editor.cursors[i].column);

                let mut cursor = Cursor::new();
                cursor.move_to(&editor.buffer, row.saturating_sub(1), col);

                editor.cursors.push(cursor);
                editor.sort_and_dedup_cursors();
            });
        }
    }

    fn add_cursor_below(&mut self) {
        let main_cursor = self.cursors[self.active_cursor_index].clone();
        let first_cursor = self.cursors.first().unwrap_or(&main_cursor);

        // If cursors have been added above, reverse them first
        if self.cursors_are_contiguous(&main_cursor, first_cursor)
            && first_cursor.row < main_cursor.row
        {
            self.remove_cursor(first_cursor.row, first_cursor.column);
        } else {
            self.for_each_cursor_rev(|editor, i| {
                let (row, col) = (editor.cursors[i].row, editor.cursors[i].column);

                let mut cursor = Cursor::new();
                cursor.move_to(&editor.buffer, row.saturating_add(1), col);

                editor.cursors.push(cursor);
                editor.sort_and_dedup_cursors();
            });
        }
    }

    fn add_cursor_at(&mut self, row: usize, col: usize) {
        let mut cursor = Cursor::new();
        cursor.move_to(&self.buffer, row, col);

        self.cursors.push(cursor);
        self.sort_and_dedup_cursors();
    }

    pub fn select_to(&mut self, row: usize, col: usize) -> ChangeSet {
        let cursor_index = self.active_cursor_index;
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(
                cursor_index,
                SelectionMode::Extend,
                CursorMode::Single,
                |c, b| c.move_to(b, row, col),
            );
        })
    }

    pub fn select_all(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.selection.begin(&Cursor::new());
            editor.cursors = vec![Cursor::new()];

            editor.cursors[0].move_to_end(&editor.buffer);
            editor.selection.update(&editor.cursors[0], &editor.buffer);
        })
    }

    pub fn clear_selection(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.selection.clear()
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
            cursors: self.cursors.clone(),
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

        self.cursors = tx.before.cursors.clone();
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

        self.cursors = tx.after.cursors.clone();
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
