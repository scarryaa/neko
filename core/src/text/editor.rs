use crate::{Buffer, Cursor, Selection};

#[derive(Default)]
pub struct Editor {
    buffer: Buffer,
    line_widths: Vec<f64>,
    max_width: f64,
    max_width_line: usize,
    cursor: Cursor,
    selection: Selection,
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
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
            self.sync_line_widths();
            self.invalidate_line_width(self.cursor.get_row());
        }

        self.cursor.insert_and_move(&mut self.buffer, text);
        self.sync_line_widths();
        self.invalidate_line_width(self.cursor.get_row());
        self.selection.clear();
    }

    pub fn insert_newline(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
            self.sync_line_widths();
        }

        let current_row = self.cursor.get_row();
        self.cursor.insert_newline(&mut self.buffer);

        // Newline adds a line, so insert width entry
        self.line_widths.insert(current_row + 1, -1.0);
        self.invalidate_line_width(current_row);

        self.selection.clear();
    }

    pub fn insert_tab(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
            self.sync_line_widths();
        }

        self.cursor.insert_tab(&mut self.buffer);
        self.invalidate_line_width(self.cursor.get_row());
        self.selection.clear();
    }

    pub fn backspace(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
            self.sync_line_widths();
        } else {
            let row_before = self.cursor.get_row();
            self.cursor.backspace_and_move(&mut self.buffer);
            let row_after = self.cursor.get_row();

            // If rows changed, a line was joined
            if row_before != row_after {
                self.line_widths.remove(row_before);
                self.invalidate_line_width(row_after);
            } else {
                self.invalidate_line_width(row_after);
            }
        }

        self.selection.clear();
    }

    pub fn delete(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
            self.sync_line_widths();
        } else {
            let line_count_before = self.buffer.line_count();
            self.cursor.delete_at_cursor(&mut self.buffer);
            let line_count_after = self.buffer.line_count();

            // If line count changed, a line was joined
            if line_count_before != line_count_after {
                self.line_widths.remove(self.cursor.get_row() + 1);
            }
            self.invalidate_line_width(self.cursor.get_row());
        }

        self.selection.clear();
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
}
