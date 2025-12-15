use super::Buffer;
use std::cmp::min;

#[derive(Debug, Default, Eq, Clone)]
pub struct Cursor {
    pub(crate) row: usize,
    pub(crate) column: usize,
    pub(crate) sticky_column: usize,
}

#[derive(Debug, Clone)]
pub struct CursorEntry {
    pub id: u64,
    pub cursor: Cursor,
    pub column_group: Option<u64>,
}

impl CursorEntry {
    pub fn individual(id: u64, cursor: &Cursor) -> Self {
        CursorEntry {
            id,
            cursor: cursor.clone(),
            column_group: None,
        }
    }

    pub fn group(id: u64, cursor: &Cursor, column_group: u64) -> Self {
        CursorEntry {
            id,
            cursor: cursor.clone(),
            column_group: Some(column_group),
        }
    }
}

impl PartialEq for Cursor {
    fn eq(&self, other: &Self) -> bool {
        self.row == other.row && self.column == other.column
    }
}

impl Cursor {
    pub fn new() -> Self {
        Self {
            row: 0,
            column: 0,
            sticky_column: 0,
        }
    }

    pub fn move_to(&mut self, buffer: &Buffer, row: usize, col: usize) {
        let max_row = buffer.line_count().saturating_sub(1).max(0);
        self.row = min(row, max_row);

        let line_len = buffer.line_len_without_newline(self.row);
        self.column = min(col, line_len);

        self.sticky_column = self.column;
    }

    pub fn move_right(&mut self, buffer: &Buffer) {
        let line_len = buffer.line_len_without_newline(self.row);

        if self.column < line_len {
            self.column += 1;
            self.sticky_column = self.column;
        } else if self.row + 1 < buffer.line_count() {
            // Wrap to start of next line
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
            // Wrap to end of previous line
            self.row -= 1;
            self.column = buffer.line_len_without_newline(self.row);
            self.sticky_column = self.column;
        }
    }

    pub fn move_down(&mut self, buffer: &Buffer) {
        if self.row + 1 < buffer.line_count() {
            self.row += 1;

            let len = buffer.line_len_without_newline(self.row);
            self.column = min(self.sticky_column, len);
        } else {
            self.move_to_end(buffer);
        }
    }

    pub fn move_up(&mut self, buffer: &Buffer) {
        if self.row > 0 {
            self.row -= 1;

            let len = buffer.line_len_without_newline(self.row);
            self.column = min(self.sticky_column, len);
        } else {
            self.move_to_start();
        }
    }

    pub fn move_to_end(&mut self, buffer: &Buffer) {
        if buffer.line_count() == 0 {
            self.row = 0;
            self.column = 0;
            self.sticky_column = 0;
            return;
        }

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

    pub fn get_idx(&self, buffer: &Buffer) -> usize {
        let line_start = buffer.line_to_byte(self.row);
        let line_len = buffer.line_len(self.row);
        let valid_col = min(self.column, line_len);

        line_start + valid_col
    }

    pub fn set_row(&mut self, row: usize) {
        self.row = row;
    }

    pub fn set_col(&mut self, col: usize) {
        self.column = col;
        self.sticky_column = col;
    }
}
