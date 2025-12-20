use super::Buffer;
use std::cmp::min;

#[derive(Debug, Default, Eq, Clone)]
pub struct Cursor {
    pub(crate) row: usize,
    pub(crate) column: usize,
    pub(crate) sticky_column: usize,
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

    pub fn from(buffer: &Buffer, row: usize, column: usize) -> Self {
        let clamped_row = Self::clamp_row(row, buffer);
        let clamped_column = Self::clamp_column(clamped_row, column, buffer);

        Self {
            row: clamped_row,
            column: clamped_column,
            sticky_column: clamped_column,
        }
    }

    // Getters
    // Row/column is clamped; byte index accounts for delimiters between lines
    pub fn get_idx(&self, buffer: &Buffer) -> usize {
        let clamped_row = Self::clamp_row(self.row, buffer);
        let clamped_column = Self::clamp_column(clamped_row, self.column, buffer);
        let line_start = buffer.line_to_byte(clamped_row);

        line_start + clamped_column
    }

    // Setters
    pub fn set_row(&mut self, buffer: &Buffer, row: usize) {
        self.row = Self::clamp_row(row, buffer);

        let len = buffer.line_len_without_newline(self.row);
        self.column = min(self.column, len);
    }

    pub fn set_column(&mut self, buffer: &Buffer, column: usize) {
        let clamped_column = Self::clamp_column(self.row, column, buffer);
        self.column = clamped_column;
        self.sticky_column = clamped_column;
    }

    // Movement methods
    pub fn move_to(&mut self, buffer: &Buffer, row: usize, column: usize) {
        let clamped_row = Self::clamp_row(row, buffer);
        let clamped_column = Self::clamp_column(clamped_row, column, buffer);

        self.row = clamped_row;
        self.column = clamped_column;
        self.sticky_column = self.column;
    }

    pub fn move_right(&mut self, buffer: &Buffer) {
        let line_len = buffer.line_len_without_newline(self.row);
        let line_count = buffer.line_count();

        if self.column < line_len {
            self.column += 1;
            self.sticky_column = self.column;
        } else if self.row + 1 < line_count {
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

            let line_length = buffer.line_len_without_newline(self.row);
            self.column = line_length;
            self.sticky_column = self.column;
        }
    }

    pub fn move_down(&mut self, buffer: &Buffer) {
        let line_count = buffer.line_count();

        if self.row + 1 < line_count {
            self.row += 1;

            let line_length = buffer.line_len_without_newline(self.row);
            self.column = min(self.sticky_column, line_length);
        } else {
            self.move_to_end(buffer);
        }
    }

    pub fn move_up(&mut self, buffer: &Buffer) {
        if self.row > 0 {
            self.row -= 1;

            let line_length = buffer.line_len_without_newline(self.row);
            self.column = min(self.sticky_column, line_length);
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

        let eol = buffer.line_len_without_newline(last_line_idx);
        self.column = eol;
        self.sticky_column = eol;
    }

    pub fn move_to_start(&mut self) {
        self.row = 0;
        self.column = 0;
        self.sticky_column = 0;
    }

    // Helpers
    fn clamp_row(row: usize, buffer: &Buffer) -> usize {
        let max_row = buffer.line_count().saturating_sub(1);
        min(row, max_row)
    }

    fn clamp_column(row: usize, column: usize, buffer: &Buffer) -> usize {
        let clamped_row = Self::clamp_row(row, buffer);
        let line_length = buffer.line_len_without_newline(clamped_row);

        min(column, line_length)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn from_correctly_clamps_row_and_column() {
        let mut b = Buffer::new();
        b.insert(0, "abc");

        let c = Cursor::from(&b, 10, 100);
        assert_eq!(c.row, 0);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn from_sets_row_column_to_zero_zero_in_empty_buffer() {
        let b = Buffer::new();
        let c = Cursor::from(&b, 10, 100);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn from_respects_desired_cursor_position_when_available() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let c = Cursor::from(&b, 2, 3);
        assert_eq!(c.row, 2);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn get_idx_calculates_correct_idx_from_row_column() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        // EOF
        let c = Cursor::from(&b, 2, 3);
        // Should be ln1 (3) + newline (1) + ln2 (3) + newline (1) + ln3 (3) = 11
        let idx = c.get_idx(&b);

        assert_eq!(idx, 11);
    }

    #[test]
    fn get_idx_calculates_correct_idx_from_row_column_without_newlines() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        // After 'c'
        let c = Cursor::from(&b, 0, 3);
        // Should be ln1 (3) = 3
        let idx = c.get_idx(&b);

        assert_eq!(idx, 3);
    }

    #[test]
    fn get_idx_calculates_correct_idx_in_empty_buffer() {
        let b = Buffer::new();

        let c = Cursor::new();

        // Should be ln1 (0) = 0
        let idx = c.get_idx(&b);

        assert_eq!(idx, 0);
    }

    #[test]
    fn get_idx_calculates_correct_idx_with_crlf() {
        let mut b = Buffer::new();
        b.insert(0, "abc\r\ndef\r\nghi");

        let c = Cursor::from(&b, 1, 2);

        // Should be ln1 (3) + newline (2) + ln2 'de' (2) = 7
        let idx = c.get_idx(&b);

        assert_eq!(idx, 7);
    }

    #[test]
    fn get_idx_calculates_correct_idx_with_lf() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let c = Cursor::from(&b, 1, 2);

        // Should be ln1 (3) + newline (1) + ln2 'de' (2) = 6
        let idx = c.get_idx(&b);

        assert_eq!(idx, 6);
    }

    #[test]
    fn set_row_clamps_row_when_past_eof() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.set_row(&b, 10);

        assert_eq!(c.row, 2);
    }

    #[test]
    fn set_row_respects_desired_row_when_available() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.set_row(&b, 1);

        assert_eq!(c.row, 1);
    }

    #[test]
    fn set_row_clamps_column_to_new_row_len() {
        let mut b = Buffer::new();
        b.insert(0, "abcdef\nx");

        let mut c = Cursor::from(&b, 0, 6);
        c.set_row(&b, 1);

        assert_eq!(c.row, 1);
        assert_eq!(c.column, 1);
    }

    #[test]
    fn set_column_clamps_column_when_past_eof() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.set_column(&b, 50);

        assert_eq!(c.column, 3);
    }

    #[test]
    fn set_column_respects_desired_column_when_available() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.set_column(&b, 2);

        assert_eq!(c.column, 2);
    }

    #[test]
    fn move_to_clamps_row_and_column_when_out_of_bounds() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.move_to(&b, 10, 10);

        assert_eq!(c.row, 2);
        // Length of last row
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_to_respects_desired_row_column_when_available() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.move_to(&b, 1, 2);

        assert_eq!(c.row, 1);
        assert_eq!(c.column, 2);
    }

    #[test]
    fn move_to_excludes_newlines() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.move_to(&b, 1, 4);

        assert_eq!(c.row, 1);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_right_increases_column_by_one_when_not_at_end_of_line() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 0, 2);
        c.move_right(&b);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_right_moves_to_the_next_line_when_at_end_of_line_and_next_line_exists() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 0, 3);
        c.move_right(&b);

        assert_eq!(c.row, 1);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_right_does_nothing_when_at_end_of_file() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 2, 3);
        c.move_right(&b);

        assert_eq!(c.row, 2);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_left_decreases_column_by_one_when_not_at_start_of_line() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 2, 3);
        c.move_left(&b);

        assert_eq!(c.row, 2);
        assert_eq!(c.column, 2);
    }

    #[test]
    fn move_left_moves_to_end_of_previous_line_when_at_start_of_current_line_and_previous_line_exists()
     {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 2, 0);
        c.move_left(&b);

        assert_eq!(c.row, 1);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_left_does_nothing_when_at_start_of_file() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.move_left(&b);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_up_decreases_row_by_one_when_not_on_first_line() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 1, 0);
        c.move_up(&b);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_up_moves_to_start_of_file_when_on_first_line() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 0, 3);
        c.move_up(&b);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_up_does_nothing_when_at_start_of_file() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::new();
        c.move_up(&b);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_up_uses_sticky_column_when_possible() {
        let mut b = Buffer::new();
        b.insert(0, "abcdef\nghi");

        let mut c = Cursor::from(&b, 1, 3);
        c.sticky_column = 5;
        c.move_up(&b);

        assert_eq!(c.row, 0);
        // Should land at 5
        assert_eq!(c.column, 5);
    }

    #[test]
    fn move_down_increases_row_by_one_when_not_on_last_line() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 1, 0);
        c.move_down(&b);

        assert_eq!(c.row, 2);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_down_moves_to_end_of_file_when_on_last_line() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 2, 0);
        c.move_down(&b);

        assert_eq!(c.row, 2);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_down_does_nothing_when_at_end_of_file() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let mut c = Cursor::from(&b, 2, 3);
        c.move_down(&b);

        assert_eq!(c.row, 2);
        assert_eq!(c.column, 3);
    }

    #[test]
    fn move_down_uses_sticky_column_when_possible() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndefghi");

        let mut c = Cursor::from(&b, 0, 3);
        c.sticky_column = 5;
        c.move_down(&b);

        assert_eq!(c.row, 1);
        // Should land at 5
        assert_eq!(c.column, 5);
    }

    #[test]
    fn move_to_end_moves_to_end_of_file() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndefghi");

        let mut c = Cursor::new();
        c.move_to_end(&b);

        assert_eq!(c.row, 1);
        assert_eq!(c.column, 6);
    }

    #[test]
    fn move_to_end_moves_to_zero_zero_when_buffer_is_empty() {
        let b = Buffer::new();
        let mut c = Cursor::from(&b, 10, 10);
        c.move_to_end(&b);

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn move_to_start_moves_to_zero_zero() {
        let b = Buffer::new();
        let mut c = Cursor::from(&b, 10, 10);
        c.move_to_start();

        assert_eq!(c.row, 0);
        assert_eq!(c.column, 0);
    }

    #[test]
    fn clamp_column_clamps_row_when_needed() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let new_column = Cursor::clamp_column(100, 2, &b);

        // NOTE: Can't verify that row is clamped directly, so we
        // rely on the column being the correct value
        assert_eq!(new_column, 2);
    }

    #[test]
    fn clamp_column_clamps_column_when_past_line_length() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let new_column = Cursor::clamp_column(0, 10, &b);

        assert_eq!(new_column, 3);
    }

    #[test]
    fn clamp_column_excludes_newlines() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let new_column = Cursor::clamp_column(0, 10, &b);

        assert_eq!(new_column, 3);
    }

    #[test]
    fn clamp_row_clamps_row_when_needed() {
        let mut b = Buffer::new();
        b.insert(0, "abc\ndef\nghi");

        let new_row = Cursor::clamp_row(100, &b);

        assert_eq!(new_row, 2);
    }

    #[test]
    fn clamp_row_clamps_row_to_zero_when_buffer_empty() {
        let b = Buffer::new();
        let new_row = Cursor::clamp_row(100, &b);

        assert_eq!(new_row, 0);
    }

    #[test]
    fn movement_never_leaves_column_past_line_len() {
        let mut b = Buffer::new();
        b.insert(0, "ab\nc");

        let mut c = Cursor::from(&b, 0, 2);

        for _ in 0..10 {
            c.move_right(&b);
            assert!(c.column <= b.line_len_without_newline(c.row));
            c.move_left(&b);
            assert!(c.column <= b.line_len_without_newline(c.row));
            c.move_down(&b);
            assert!(c.column <= b.line_len_without_newline(c.row));
            c.move_up(&b);
            assert!(c.column <= b.line_len_without_newline(c.row));
        }
    }

    #[test]
    fn get_idx_never_exceeds_buffer_len() {
        let mut b = Buffer::new();
        b.insert(0, "ab\ncd\n");

        for row in 0..=100 {
            for col in 0..=100 {
                let c = Cursor::from(&b, row, col);
                assert!(c.get_idx(&b) <= b.byte_len());
            }
        }
    }
}
