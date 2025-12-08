use std::cmp::min;

use super::Buffer;

#[derive(Debug, Default, PartialEq, Eq, Clone)]
pub struct Cursor {
    row: usize,
    column: usize,
    sticky_column: usize,
    idx: usize,
}

impl Cursor {
    pub fn new() -> Self {
        Self {
            row: 0,
            column: 0,
            sticky_column: 0,
            idx: 1,
        }
    }

    pub fn insert_and_move(&mut self, buffer: &mut Buffer, text: &str) {
        buffer.insert(self.idx - 1, text);

        self.idx += text.len();
        let newlines = text.chars().filter(|&c| c == '\n').count();

        if newlines == 0 {
            self.column += text.len();
        } else {
            self.row += newlines;
            if let Some(last_line) = text.split('\n').next_back() {
                self.column = last_line.len();
            }
        }

        self.sticky_column = self.column;
    }

    pub fn insert_newline(&mut self, buffer: &mut Buffer) {
        buffer.insert(self.idx - 1, "\n");

        self.row += 1;
        self.column = 0;
        self.sticky_column = 0;
        self.idx += 1;
    }

    pub fn insert_tab(&mut self, buffer: &mut Buffer) {
        // TODO: Make this configurable
        let tab_width = 4;
        let spaces = " ".repeat(tab_width);

        buffer.insert(self.idx - 1, &spaces);

        self.column += tab_width;
        self.sticky_column += self.column;
        self.idx += tab_width;
    }

    pub fn backspace_and_move(&mut self, buffer: &mut Buffer) {
        if self.idx - 1 > 0 {
            self.move_left(buffer);
            buffer.backspace(self.idx);
        }
    }

    pub fn delete_at_cursor(&mut self, buffer: &mut Buffer) {
        if self.idx - 1 < buffer.byte_len() {
            buffer.delete_at(self.idx - 1);
        }
    }

    pub fn move_right(&mut self, buffer: &Buffer) {
        if self.row >= buffer.line_count() {
            return;
        }

        if self.column < buffer.line_len(self.row) {
            self.column += 1;
            self.sticky_column = self.column;
            self.idx += 1;
        } else if self.row + 1 < buffer.line_count() {
            self.row += 1;
            self.column = 0;
            self.sticky_column = 0;
            self.idx += 1;
        }
    }

    pub fn move_left(&mut self, buffer: &Buffer) {
        if self.column > 0 {
            self.column -= 1;
            self.sticky_column = self.column;
            self.idx -= 1;
        } else if self.row > 0 {
            self.row -= 1;
            self.column = buffer.line_len(self.row);
            self.sticky_column = buffer.line_len(self.row);
            self.idx -= 1;
        }
    }

    pub fn move_down(&mut self, buffer: &Buffer) {
        let prev_col = self.column;
        let prev_row = self.row;

        if self.row + 1 < buffer.line_count() {
            self.row += 1;

            let line_len = buffer.line_len(self.row);

            self.column = min(line_len, self.sticky_column);
        } else if self.row + 1 == buffer.line_count() {
            let line_len = buffer.line_len(self.row);
            self.column = line_len;
            self.sticky_column = line_len;
        }

        if prev_row != self.row {
            // Moved to next line
            let prev_line_len = buffer.line_len(prev_row);

            // +1 for newline
            self.idx = self.idx + prev_line_len - prev_col + self.column + 1;
        } else {
            // Moved within the same line
            self.idx += self.column - prev_col;
        }
    }

    pub fn move_up(&mut self, buffer: &Buffer) {
        let prev_col = self.column;
        let prev_row = self.row;

        if self.row > 0 {
            self.row -= 1;

            let line_len = buffer.line_len(self.row);
            self.column = min(line_len, self.sticky_column);
        } else {
            self.row = 0;
            self.column = 0;
            self.sticky_column = 0;
        }

        if prev_row != self.row {
            // Moved to previous line
            let new_line_len = buffer.line_len(self.row);

            // -1 for newline
            self.idx = self.idx - prev_col - (new_line_len - self.column) - 1;
        } else {
            // Moved within the same line
            self.idx -= prev_col - self.column;
        }
    }

    pub fn move_to_end(&mut self, buffer: &Buffer) {
        let last_line_idx = buffer.line_count() - 1;
        self.row = last_line_idx;
        self.column = buffer.line_len(last_line_idx);
        self.sticky_column = buffer.line_len(last_line_idx);
        self.idx = buffer.byte_len();
    }

    pub fn move_to_start(&mut self) {
        self.row = 0;
        self.column = 0;
        self.sticky_column = 0;
        self.idx = 0;
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

    pub fn get_idx(&self) -> usize {
        self.idx - 1
    }
}
