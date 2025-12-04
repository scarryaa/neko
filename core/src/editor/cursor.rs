use super::Buffer;

#[derive(Debug, Default)]
pub struct Cursor {
    row: usize,
    column: usize,
}

impl Cursor {
    pub fn new() -> Self {
        Self { row: 0, column: 0 }
    }

    pub fn move_right(&mut self, buffer: &Buffer) {
        if self.row > buffer.line_count() {
            return;
        }

        if self.column < buffer.line_len(self.row) {
            self.column += 1;
        } else {
            self.row += 1;
        }
    }

    pub fn move_left(&mut self, _buffer: &Buffer) {
        if self.row == 0 && self.column == 0 {
            return;
        }

        if self.column == 0 {
            self.row -= 1;
        } else {
            self.column -= 1;
        }
    }

    pub fn move_up(&mut self, buffer: &Buffer) {
        if self.row == 0 {
            self.column = 0;
            return;
        }

        self.row -= 1;
        let prev_line_len = buffer.line_len(self.row);

        if self.column > prev_line_len {
            self.column = prev_line_len;
        }
    }

    pub fn move_down(&mut self, buffer: &Buffer) {
        let line_count = buffer.line_count();
        if self.row >= line_count {
            self.column = buffer.line_len(line_count);
            return;
        }

        self.row += 1;
        let next_line_len = buffer.line_len(self.row);

        if self.column > next_line_len {
            self.column = next_line_len;
        }
    }

    pub fn get_row(&self) -> usize {
        self.row
    }

    pub fn get_col(&self) -> usize {
        self.column
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

