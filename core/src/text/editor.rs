use std::fmt::Debug;

use crate::{Buffer, Cursor, Selection};

use super::{Edit, Transaction, UndoHistory, ViewState};

#[derive(Default, Debug)]
pub struct Editor {
    buffer: Buffer,
    line_widths: Vec<f64>,
    max_width: f64,
    max_width_line: usize,
    cursor: Cursor,
    selection: Selection,
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

    pub fn buffer(&self) -> &Buffer {
        &self.buffer
    }

    pub fn cursor(&self) -> &Cursor {
        &self.cursor
    }

    pub fn max_width(&self) -> f64 {
        self.max_width
    }

    pub fn selection(&self) -> &Selection {
        &self.selection
    }

    pub fn load_file(&mut self, content: &str) {
        self.buffer.clear();
        self.buffer.insert(0, content);
        self.cursor = Cursor::new();
        self.selection = Selection::new();

        // Resize line_widths to match buffer
        let line_count = self.buffer.line_count();
        self.line_widths.clear();
        self.line_widths.resize(line_count, -1.0);
        self.max_width = 0.0;
        self.max_width_line = 0;
    }

    pub fn insert_text(&mut self, text: &str) {
        self.begin_tx();

        if self.selection.is_active() {
            let start = self.selection.start().get_idx();
            let end = self.selection.end().get_idx();

            self.cursor = self.selection.start().clone();

            let deleted = self.buffer.delete_range_capture(start, end);
            self.record_edit(Edit::Delete {
                start,
                end,
                deleted,
            });

            self.selection.clear();
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.get_row());
        }

        let pos = self.cursor.get_idx();
        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        self.cursor.move_right_by_bytes(&self.buffer, text.len());
        self.sync_line_widths();
        self.invalidate_line_width(self.cursor.get_row());

        self.commit_tx();
    }

    pub fn insert_newline(&mut self) {
        self.begin_tx();

        if self.selection.is_active() {
            let start = self.selection.start().get_idx();
            let end = self.selection.end().get_idx();

            self.cursor = self.selection.start().clone();

            let deleted = self.buffer.delete_range_capture(start, end);
            self.record_edit(Edit::Delete {
                start,
                end,
                deleted,
            });

            self.selection.clear();
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.get_row());
        }

        let pos = self.cursor.get_idx();
        let text = "\n";
        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let current_row = self.cursor.get_row();
        self.cursor.move_newline();

        // Newline adds a line, so insert width entry
        self.line_widths.insert(current_row + 1, -1.0);
        self.invalidate_line_width(current_row);

        self.commit_tx();
    }

    pub fn insert_tab(&mut self) {
        self.begin_tx();

        if self.selection.is_active() {
            let start = self.selection.start().get_idx();
            let end = self.selection.end().get_idx();

            self.cursor = self.selection.start().clone();

            let deleted = self.buffer.delete_range_capture(start, end);
            self.record_edit(Edit::Delete {
                start,
                end,
                deleted,
            });

            self.selection.clear();
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.get_row());
        }

        let pos = self.cursor.get_idx();
        let text = "    ";
        self.buffer.insert(pos, text);
        self.record_edit(Edit::Insert {
            pos,
            text: text.to_string(),
        });

        let current_row = self.cursor.get_row();
        self.cursor.move_tab();

        self.invalidate_line_width(current_row);

        self.commit_tx();
    }

    pub fn backspace(&mut self) {
        self.begin_tx();

        if self.selection.is_active() {
            let start = self.selection.start().get_idx();
            let end = self.selection.end().get_idx();

            self.cursor = self.selection.start().clone();
            let deleted = self.buffer.delete_range_capture(start, end);
            self.record_edit(Edit::Delete {
                start,
                end,
                deleted,
            });

            self.selection.clear();
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.get_row());
        } else {
            let pos = self.cursor.get_idx();
            let row_before = self.cursor.get_row();

            if pos > 0 {
                let start = pos - 1;
                let end = pos;

                let deleted = self.buffer.delete_range_capture(start, end);
                self.record_edit(Edit::Delete {
                    start,
                    end,
                    deleted,
                });

                self.cursor.move_left(&self.buffer);
                let row_after = self.cursor.get_row();

                if row_before != row_after {
                    self.line_widths.remove(row_before);
                }
                self.invalidate_line_width(row_after);
            }
        }

        self.commit_tx();
    }

    pub fn delete(&mut self) {
        self.begin_tx();

        if self.selection.is_active() {
            let start = self.selection.start().get_idx();
            let end = self.selection.end().get_idx();

            self.cursor = self.selection.start().clone();
            let deleted = self.buffer.delete_range_capture(start, end);
            self.record_edit(Edit::Delete {
                start,
                end,
                deleted,
            });

            self.selection.clear();
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.get_row());
        } else {
            let start = self.cursor.get_idx();

            let line_count_before = self.buffer.line_count();
            let deleted = self.buffer.delete_at_capture(start);
            let line_count_after = self.buffer.line_count();

            if !deleted.is_empty() {
                let end = start + deleted.len();
                self.record_edit(Edit::Delete {
                    start,
                    end,
                    deleted,
                });
            }

            // If line count changed, a line was joined
            if line_count_before != line_count_after {
                self.line_widths.remove(self.cursor.get_row() + 1);
            }
            self.invalidate_line_width(self.cursor.get_row());
        }

        self.commit_tx();
    }

    pub fn move_to(&mut self, row: usize, col: usize, clear_selection: bool) {
        self.cursor.move_to(&self.buffer, row, col);

        if clear_selection {
            self.selection.clear();
        }
    }

    pub fn move_left(&mut self) {
        self.cursor.move_left(&self.buffer);
        self.selection.clear();
    }

    pub fn move_right(&mut self) {
        self.cursor.move_right(&self.buffer);
        self.selection.clear();
    }

    pub fn move_up(&mut self) {
        self.cursor.move_up(&self.buffer);
        self.selection.clear();
    }

    pub fn move_down(&mut self) {
        self.cursor.move_down(&self.buffer);
        self.selection.clear();
    }

    pub fn select_to(&mut self, row: usize, col: usize) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }
        self.move_to(row, col, false);
        self.selection.update(&self.cursor);
    }

    pub fn select_left(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_left(&self.buffer);
        self.selection.update(&self.cursor);
    }

    pub fn select_right(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_right(&self.buffer);
        self.selection.update(&self.cursor);
    }

    pub fn select_up(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_up(&self.buffer);
        self.selection.update(&self.cursor);
    }

    pub fn select_down(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_down(&self.buffer);
        self.selection.update(&self.cursor);
    }

    pub fn select_all(&mut self) {
        self.selection.begin(&Cursor::new());
        self.cursor.move_to_end(&self.buffer);
        self.selection.update(&self.cursor);
    }

    pub fn clear_selection(&mut self) {
        self.selection.clear();
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

    pub fn undo(&mut self) {
        let Some(tx) = self.history.undo.pop() else {
            return;
        };

        for edit in tx.edits.iter().rev() {
            edit.invert().apply(&mut self.buffer);
        }

        self.cursor = tx.before.cursor.clone();
        self.selection = tx.before.selection.clone();

        self.sync_line_widths();
        for i in 0..self.line_widths.len() {
            self.line_widths[i] = -1.0;
        }
        self.recalculate_max_width();

        self.history.redo.push(tx);
    }

    pub fn redo(&mut self) {
        let Some(tx) = self.history.redo.pop() else {
            return;
        };

        for edit in tx.edits.iter() {
            edit.apply(&mut self.buffer);
        }

        self.cursor = tx.after.cursor.clone();
        self.selection = tx.after.selection.clone();

        self.sync_line_widths();
        for i in 0..self.line_widths.len() {
            self.line_widths[i] = -1.0;
        }
        self.recalculate_max_width();

        self.history.undo.push(tx);
    }
}
