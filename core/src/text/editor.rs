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

#[derive(Default, Debug)]
pub struct Editor {
    pub(crate) buffer: Buffer,
    line_widths: Vec<f64>,
    pub(crate) max_width: f64,
    max_width_line: usize,
    pub(crate) cursors: Vec<Cursor>,
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
            selection: Selection::new(),
            history: UndoHistory::default(),
        }
    }

    fn sort_and_dedup_cursors(&mut self) {
        self.cursors.sort_by(|c1, c2| {
            if c1.row == c2.row {
                c1.column.cmp(&c2.column)
            } else {
                c1.row.cmp(&c2.row)
            }
        });
        self.cursors.dedup();
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

        self.cursors = vec![self.selection.start.clone()];

        let deleted = self.buffer.delete_range(start, end);
        self.clear_and_rebuild_line_widths();
        self.record_edit(Edit::Delete {
            start,
            end,
            deleted,
        });

        self.selection.clear();
        self.sync_line_widths();
        self.invalidate_line_width(self.cursors[0].row);
    }

    pub fn insert_text(&mut self, text: &str) -> ChangeSet {
        self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            let has_nl = text.contains('\n');
            let is_tab = text == "\t";

            for i in (0..editor.cursors.len()).rev() {
                if has_nl {
                    editor.insert_newline(text, i);
                } else if is_tab {
                    editor.insert_tab(i);
                } else {
                    editor.insert_text_internal(text, i);
                }
            }
        })
    }

    fn insert_text_internal(&mut self, text: &str, i: usize) {
        self.delete_selection_if_active();

        let pos = self.cursors[i].get_idx(&self.buffer);

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let row = self.cursors[i].row;
        let col = self.cursors[i].column + text.len();

        self.cursors[i].move_to(&self.buffer, row, col);

        self.sync_line_widths();
        self.invalidate_line_width(self.cursors[i].row);
    }

    fn insert_newline(&mut self, text: &str, i: usize) {
        self.delete_selection_if_active();

        let pos = self.cursors[i].get_idx(&self.buffer);

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let current_row = self.cursors[i].row;
        let inserted_newlines = text.matches('\n').count();
        let new_row = current_row + inserted_newlines;

        let last = text.rsplit('\n').next().unwrap_or("");
        let last = last.strip_suffix('\r').unwrap_or(last);
        let new_col = last.len();

        self.cursors[i].move_to(&self.buffer, new_row, new_col);
        self.sync_line_widths();
        self.invalidate_line_width(current_row);
    }

    fn insert_tab(&mut self, i: usize) {
        self.delete_selection_if_active();

        let text = "    ";
        let pos = self.cursors[i].get_idx(&self.buffer);

        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let (row, col) = (self.cursors[i].row, self.cursors[i].column + text.len());
        self.cursors[i].move_to(&self.buffer, row, col);

        self.sync_line_widths();
        self.invalidate_line_width(self.cursors[i].row);
    }

    pub fn backspace(&mut self) -> ChangeSet {
        self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.backspace_impl(i);
            }
        })
    }

    fn backspace_impl(&mut self, i: usize) {
        if self.delete_selection_if_active() {
            self.sync_line_widths();
            return;
        }

        let pos = self.cursors[i].get_idx(&self.buffer);
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

        self.cursors[i].move_left(&self.buffer);

        self.sync_line_widths();
        self.invalidate_line_width(self.cursors[i].row);
    }

    pub fn delete(&mut self) -> ChangeSet {
        self.with_op(true, OpFlags::BufferViewportWidths, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.delete_impl(i)
            }
        })
    }

    fn delete_impl(&mut self, i: usize) {
        if self.delete_selection_if_active() {
            self.sync_line_widths();
            self.invalidate_line_width(self.cursors[i].row);
            return;
        }

        let start = self.cursors[i].get_idx(&self.buffer);
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
            self.invalidate_line_width(self.cursors[i].row);
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
        let cursor = self.cursors[0].clone();

        if let SelectionMode::Extend = mode {
            if !self.selection.is_active() {
                self.selection.begin(&cursor);
            }
        }

        match cursor_mode {
            CursorMode::Single => {
                self.cursors.clear();
                self.cursors.push(cursor);
                f(&mut self.cursors[0], &self.buffer);
            }
            CursorMode::Multiple => f(&mut self.cursors[i], &self.buffer),
        }

        match mode {
            SelectionMode::Extend => self.selection.update(&self.cursors[0], &self.buffer),
            SelectionMode::Clear => self.selection.clear(),
            SelectionMode::Keep => {}
        }
    }

    pub fn move_left(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_left(b)
                });
            }
        })
    }

    pub fn move_right(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_right(b)
                });
            }
        })
    }

    pub fn move_up(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_up(b)
                });
            }
        })
    }

    pub fn move_down(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Clear, CursorMode::Multiple, |c, b| {
                    c.move_down(b)
                });
            }
        })
    }

    pub fn select_left(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                    c.move_left(b)
                });
            }
        })
    }

    pub fn select_right(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                    c.move_right(b)
                });
            }
        })
    }

    pub fn select_up(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                    c.move_up(b)
                });
            }
        })
    }

    pub fn select_down(&mut self) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            for i in (0..editor.cursors.len()).rev() {
                editor.move_cursor_at(i, SelectionMode::Extend, CursorMode::Single, |c, b| {
                    c.move_down(b)
                });
            }
        })
    }

    pub fn move_to(&mut self, row: usize, col: usize, clear: bool) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            let mode = if clear {
                SelectionMode::Clear
            } else {
                SelectionMode::Keep
            };
            editor.move_cursor_at(0, mode, CursorMode::Single, |c, b| c.move_to(b, row, col));
        })
    }

    pub fn add_cursor(&mut self, row: usize, col: usize) {
        let mut cursor = Cursor::new();
        cursor.move_to(&self.buffer, row, col);

        self.cursors.push(cursor);
        self.sort_and_dedup_cursors();
    }

    pub fn select_to(&mut self, row: usize, col: usize) -> ChangeSet {
        self.with_op(false, OpFlags::ViewportOnly, |editor| {
            editor.move_cursor_at(0, SelectionMode::Extend, CursorMode::Single, |c, b| {
                c.move_to(b, row, col)
            });
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
