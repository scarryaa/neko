use std::cmp::min;

use super::Buffer;

#[derive(Debug, Default, PartialEq, Eq, Clone)]
pub struct Cursor {
    row: usize,
    column: usize,
    sticky_column: usize,
}

impl Cursor {
    pub fn new() -> Self {
        Self {
            row: 0,
            column: 0,
            sticky_column: 0,
        }
    }

    pub fn insert_and_move(&mut self, buffer: &mut Buffer, text: &str) {
        let idx = self.get_idx(buffer);
        buffer.insert(idx, text);

        // TODO: Make this faster
        for ch in text.chars() {
            if ch == '\n' {
                self.row += 1;
                self.column = 0;
                self.sticky_column = 0;
            } else {
                self.column += 1;
                self.sticky_column = self.column;
            }
        }
    }

    pub fn insert_newline(&mut self, buffer: &mut Buffer) {
        let idx = self.get_idx(buffer);
        buffer.insert(idx, "\n");

        self.row += 1;
        self.column = 0;
        self.sticky_column = 0;
    }

    pub fn insert_tab(&mut self, buffer: &mut Buffer) {
        let idx = self.get_idx(buffer);
        // TODO: Make this configurable
        let tab_width = 4;
        let spaces = " ".repeat(tab_width);

        buffer.insert(idx, &spaces);

        self.column += tab_width;
        self.sticky_column += self.column;
    }

    pub fn backspace_and_move(&mut self, buffer: &mut Buffer) {
        let idx = self.get_idx(buffer);

        if idx > 0 {
            self.move_left(buffer);
            buffer.backspace(idx);
        }
    }

    pub fn delete_at_cursor(&mut self, buffer: &mut Buffer) {
        let idx = self.get_idx(buffer);

        if idx < buffer.byte_len() {
            buffer.delete_at(idx);
        }
    }

    pub fn move_right(&mut self, buffer: &Buffer) {
        if self.row >= buffer.line_count() {
            return;
        }

        if self.column < buffer.line_len(self.row) {
            self.column += 1;
            self.sticky_column = self.column;
        } else if self.row + 1 < buffer.line_count() {
            self.row += 1;
            self.column = 0;
            self.sticky_column = 0;
        }
    }

    pub fn move_left(&mut self, buffer: &Buffer) {
        if self.column > 0 {
            self.column -= 1;
            self.sticky_column = self.column;
        } else if self.row > 0 {
            self.row -= 1;
            self.column = buffer.line_len(self.row);
            self.sticky_column = buffer.line_len(self.row);
        }
    }

    pub fn move_down(&mut self, buffer: &Buffer) {
        if self.row + 1 < buffer.line_count() {
            self.row += 1;

            let line_len = buffer.line_len(self.row);

            self.column = min(line_len, self.sticky_column);
        } else if self.row + 1 == buffer.line_count() {
            let line_len = buffer.line_len(self.row);
            self.column = line_len;
            self.sticky_column = line_len;
        }
    }

    pub fn move_up(&mut self, buffer: &Buffer) {
        if self.row > 0 {
            self.row -= 1;

            let line_len = buffer.line_len(self.row);
            self.column = min(line_len, self.sticky_column);
        } else {
            self.row = 0;
            self.column = 0;
            self.sticky_column = 0;
        }
    }

    pub fn move_to_end(&mut self, buffer: &Buffer) {
        let last_line_idx = buffer.line_count() - 1;
        self.row = last_line_idx;
        self.column = buffer.line_len(last_line_idx);
        self.sticky_column = buffer.line_len(last_line_idx);
    }

    pub fn move_to_start(&mut self) {
        self.row = 0;
        self.column = 0;
        self.sticky_column = 0;
    }

    pub fn set_row(&mut self, row: usize) {
        self.row = row;
    }

    pub fn set_col(&mut self, col: usize) {
        self.column = col;
        self.sticky_column = col;
    }

    pub fn get_row(&self) -> usize {
        self.row
    }

    pub fn get_col(&self) -> usize {
        self.column
    }

    pub fn get_sticky_col(&self) -> usize {
        self.sticky_column
    }

    pub fn get_idx(&self, buffer: &Buffer) -> usize {
        let mut total = self.column;

        for row in 0..self.row {
            total += buffer.line_len(row);
            total += 1; // Add 1 for the newline character
        }

        total
    }
}
