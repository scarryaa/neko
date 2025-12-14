use std::fmt::Debug;

use crate::{Buffer, Cursor, Selection};

use super::{Change, ChangeSet, Edit, Transaction, UndoHistory, ViewState, change_set::OpFlags};

enum SelectionMode {
    Clear,
    Extend,
    Keep,
}

#[derive(Default, Debug)]
pub struct Editor {
    pub(crate) buffer: Buffer,
    line_widths: Vec<f64>,
    pub(crate) max_width: f64,
    max_width_line: usize,
    pub(crate) cursor: Cursor,
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
            cursor: Cursor::new(),
            selection: Selection::new(),
            history: UndoHistory::default(),
        }
    }

    fn with_op<R>(
        &mut self,
        tx: bool,
        flags: OpFlags,
        f: impl FnOnce(&mut Self) -> R,
    ) -> ChangeSet {
        let (lc0, cur0, sel0) = self.begin_changes();
        if tx {
            self.begin_tx();
        }

        f(self);

        if tx {
            self.commit_tx();
        }

        let mut cs = self.end_changes(lc0, &cur0, &sel0);
        match flags {
            OpFlags::ViewportOnly => cs.change |= Change::VIEWPORT,
            OpFlags::BufferViewportWidths => {
                cs.change |= Change::BUFFER | Change::VIEWPORT | Change::WIDTHS
            }
        }

        cs
    }

    fn begin_changes(&self) -> (usize, Cursor, Selection) {
        (
            self.buffer.line_count(),
            self.cursor.clone(),
            self.selection.clone(),
        )
    }

    fn end_changes(
        &self,
        before_line_count: usize,
        before_cursor: &Cursor,
        before_selection: &Selection,
    ) -> ChangeSet {
        let after_line_count = self.buffer.line_count();

        let mut c = Change::NONE;
        if after_line_count != before_line_count {
            c |= Change::LINE_COUNT | Change::VIEWPORT;
        }
        if &self.cursor != before_cursor {
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
        self.cursor = Cursor::new();
        self.selection = Selection::new();

        self.clear_and_rebuild_line_widths();

        let mut cs = self.end_changes(lc0, &cur0, &sel0);
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

        self.cursor = self.selection.start.clone();

        let deleted = self.buffer.delete_range(start, end);
        self.clear_and_rebuild_line_widths();
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection.clear();
        self.sync_line_widths();
        self.invalidate_line_width(self.cursor.row);
    }

    pub fn insert_text(&mut self, text: &str) -> ChangeSet {
        self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            if text.contains('\n') {
                editor.insert_newline(text);
            } else if text == "\t" {
                editor.insert_tab();
            } else {
                editor.insert_text_internal(text);
            }
        })
    }

    fn insert_text_internal(&mut self, text: &str) {
        self.delete_selection_if_active();

        let pos = self.cursor.get_idx(&self.buffer);

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });
        self.cursor.move_to(
            &self.buffer,
            self.cursor.row,
            self.cursor.column + text.len(),
        );

        self.sync_line_widths();
        self.invalidate_line_width(self.cursor.row);
    }

    fn insert_newline(&mut self, text: &str) {
        self.delete_selection_if_active();

        let pos = self.cursor.get_idx(&self.buffer);

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let current_row = self.cursor.row;
        let inserted_newlines = text.matches('\n').count();
        let new_row = current_row + inserted_newlines;

        let last = text.rsplit('\n').next().unwrap_or("");
        let last = last.strip_suffix('\r').unwrap_or(last);
        let new_col = last.len();

        self.cursor.move_to(&self.buffer, new_row, new_col);
        self.sync_line_widths();
        self.invalidate_line_width(current_row);
    }

    fn insert_tab(&mut self) {
        self.delete_selection_if_active();

        let text = "    ";
        let pos = self.cursor.get_idx(&self.buffer);

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        self.cursor.move_to(
            &self.buffer,
            self.cursor.row,
            self.cursor.column + text.len(),
        );

        self.sync_line_widths();
        self.invalidate_line_width(self.cursor.row);
    }

    pub fn backspace(&mut self) -> ChangeSet {
        self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            editor.backspace_impl();
        })
    }

    fn backspace_impl(&mut self) {
        if self.delete_selection_if_active() {
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.row);
            return;
        }

        let pos = self.cursor.get_idx(&self.buffer);
        if pos == 0 {
            return;
        }

        let start = pos - 1;
        let end = pos;
        let deleted = self.buffer.delete_range(start, end);
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.cursor.move_left(&self.buffer);

        self.sync_line_widths();
        self.invalidate_line_width(self.cursor.row);
    }

    pub fn delete(&mut self) -> ChangeSet {
        self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            editor.delete_impl()
        })
    }

    fn delete_impl(&mut self) {
        if self.delete_selection_if_active() {
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.row);
            return;
        }

        let start = self.cursor.get_idx(&self.buffer);
        if start < self.buffer.byte_len() - 1 {
            let deleted = self.buffer.delete_at(start);

            if !deleted.is_empty() {
                let end = start + deleted.len();
                self.record_edit(Edit::Delete {
                    start,
                    end,
                    deleted,
                });
            }

            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.row);
        }
    }

    fn move_cursor(&mut self, mode: SelectionMode, f: impl FnOnce(&mut Cursor, &Buffer)) {
        if let SelectionMode::Extend = mode {
            if !self.selection.is_active() {
                self.selection.begin(&self.cursor);
            }
        }

        f(&mut self.cursor, &self.buffer);

        match mode {
            SelectionMode::Extend => self.selection.update(&self.cursor, &self.buffer),
            SelectionMode::Clear => self.selection.clear(),
            SelectionMode::Keep => {}
        }
    }

    pub fn move_left(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Clear, |c, b| c.move_left(b));
        })
    }

    pub fn move_right(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Clear, |c, b| c.move_right(b));
        })
    }

    pub fn move_up(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Clear, |c, b| c.move_up(b));
        })
    }

    pub fn move_down(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Clear, |c, b| c.move_down(b));
        })
    }

    pub fn select_left(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Extend, |c, b| c.move_left(b));
        })
    }

    pub fn select_right(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Extend, |c, b| c.move_right(b));
        })
    }

    pub fn select_up(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Extend, |c, b| c.move_up(b));
        })
    }

    pub fn select_down(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Extend, |c, b| c.move_down(b));
        })
    }

    pub fn move_to(&mut self, row: usize, col: usize, clear: bool) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            let mode = if clear {
                SelectionMode::Clear
            } else {
                SelectionMode::Keep
            };
            editor.move_cursor(mode, |c, b| c.move_to(b, row, col));
        })
    }

    pub fn select_to(&mut self, row: usize, col: usize) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor(SelectionMode::Extend, |c, b| c.move_to(b, row, col));
        })
    }

    pub fn select_all(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.selection.begin(&Cursor::new());
            editor.cursor.move_to_end(&editor.buffer);
            editor.selection.update(&editor.cursor, &editor.buffer);
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
            cursor: self.cursor.clone(),
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

        self.cursor = tx.before.cursor.clone();
        self.selection = tx.before.selection.clone();

        self.clear_and_rebuild_line_widths();

        self.history.redo.push(tx);

        let mut cs = self.end_changes(lc0, &cur0, &sel0);
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

        self.cursor = tx.after.cursor.clone();
        self.selection = tx.after.selection.clone();

        self.clear_and_rebuild_line_widths();

        self.history.undo.push(tx);

        let mut cs = self.end_changes(lc0, &cur0, &sel0);
        cs.change |= Change::SELECTION
            | Change::BUFFER
            | Change::CURSOR
            | Change::VIEWPORT
            | Change::WIDTHS
            | Change::LINE_COUNT;
        cs
    }
}
