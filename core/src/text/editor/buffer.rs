use crop::Rope;
use std::mem::swap;

#[derive(Debug, Default)]
pub struct Buffer {
    content: Rope,
}

impl Buffer {
    pub fn new() -> Self {
        Self {
            content: Rope::new(),
        }
    }

    pub fn from(content: &str) -> Self {
        Self {
            content: Rope::from(content),
        }
    }

    // Getters
    pub fn is_empty(&self) -> bool {
        self.content.is_empty()
    }

    pub fn get_text(&self) -> String {
        self.content.to_string()
    }

    pub fn get_line(&self, line_idx: usize) -> String {
        let base = self.content.line_len();

        if self.content.is_empty() {
            return String::new();
        }

        if self.ends_with_newline() && line_idx == base {
            return String::new();
        }

        if line_idx >= base {
            return String::new();
        }

        self.content.line(line_idx).to_string()
    }

    pub fn get_lines(&self) -> Vec<String> {
        (0..self.line_count()).map(|i| self.get_line(i)).collect()
    }

    pub fn get_text_range(&self, start_idx: usize, end_idx: usize) -> String {
        let max_idx = self.content.byte_len();
        let mut clamped_start = start_idx.clamp(0, max_idx);
        let mut clamped_end = end_idx.clamp(0, max_idx);

        if clamped_start > clamped_end {
            swap(&mut clamped_start, &mut clamped_end);
        }

        self.content
            .byte_slice(clamped_start..clamped_end)
            .to_string()
    }

    pub fn line_len(&self, line_idx: usize) -> usize {
        let base = self.content.line_len();

        if self.content.is_empty() {
            return 0;
        }

        if self.ends_with_newline() && line_idx == base {
            return 0;
        }

        if line_idx >= base {
            return 0;
        }

        self.content.line(line_idx).byte_len()
    }

    pub fn line_len_without_newline(&self, line_idx: usize) -> usize {
        if line_idx >= self.content.line_len() {
            return 0;
        }

        let line = self.content.line(line_idx);
        let len = line.byte_len();

        // Check for \r\n or \n
        if len >= 2 && line.byte_slice(len - 2..len) == "\r\n" {
            return len - 2;
        } else if len >= 1 && line.byte_slice(len - 1..len) == "\n" {
            return len - 1;
        }
        len
    }

    pub fn line_to_byte(&self, line_idx: usize) -> usize {
        let base = self.content.line_len();

        if self.content.is_empty() {
            return 0;
        }

        if self.ends_with_newline() && line_idx == base {
            return self.content.byte_len();
        }

        if line_idx >= base {
            return 0;
        }

        self.content.byte_of_line(line_idx)
    }

    pub fn byte_to_line(&self, byte_idx: usize) -> usize {
        if self.content.line_len() == 0 {
            return 0;
        }

        self.content.line_of_byte(byte_idx)
    }

    pub fn byte_to_row_col(&self, byte_idx: usize) -> (usize, usize) {
        let len = self.content.byte_len();

        if len == 0 {
            return (0, 0);
        }

        let byte_idx = byte_idx.min(len);

        if byte_idx == len && self.ends_with_newline() {
            let row = self.content.line_len();
            return (row, 0);
        }

        let row = self.content.line_of_byte(byte_idx);
        let line_start = self.line_to_byte(row);
        let col = byte_idx.saturating_sub(line_start);
        let col = col.min(self.line_len_without_newline(row));
        (row, col)
    }

    pub fn byte_len(&self) -> usize {
        self.content.byte_len()
    }

    pub fn line_count(&self) -> usize {
        self.visual_line_count()
    }

    pub fn visual_line_count(&self) -> usize {
        if self.content.is_empty() {
            return 1;
        }
        let base = self.content.line_len();

        if self.ends_with_newline() {
            base + 1
        } else {
            base
        }
    }

    fn ends_with_lf(&self) -> bool {
        let len = self.content.byte_len();

        if len >= 1 {
            let ends_with_lf = self.content.byte_slice(len - 1..len) == "\n";

            if len >= 2 {
                let ends_with_crlf = self.content.byte_slice(len - 2..len) == "\r\n";
                !ends_with_crlf && ends_with_lf
            } else {
                ends_with_lf
            }
        } else {
            false
        }
    }

    fn ends_with_crlf(&self) -> bool {
        let len = self.content.byte_len();
        len >= 2 && self.content.byte_slice(len - 2..len) == "\r\n"
    }

    fn ends_with_newline(&self) -> bool {
        self.ends_with_lf() || self.ends_with_crlf()
    }

    // CRUD
    pub fn insert(&mut self, pos: usize, text: &str) {
        if pos <= self.content.byte_len() {
            self.content.insert(pos, text);
        }
    }

    pub fn clear(&mut self) {
        self.content.delete(0..);
    }

    pub fn backspace(&mut self, pos: usize) {
        if pos == 0 {
            return;
        }

        self.content.delete(pos - 1..pos);
    }

    pub fn delete_at(&mut self, pos: usize) -> String {
        let deleted = self.get_text_range(pos, pos + 1);

        if pos < self.content.byte_len() {
            self.content.delete(pos..pos + 1);
            deleted
        } else {
            String::new()
        }
    }

    pub fn delete_range(&mut self, start_pos: usize, end_pos: usize) -> String {
        let len = self.content.byte_len();
        let mut start = start_pos.min(len);
        let mut end = end_pos.min(len);

        if start > end {
            swap(&mut start, &mut end);
        }

        let deleted = self.content.byte_slice(start..end).to_string();

        if start != end {
            self.content.delete(start..end);
        }

        deleted
    }
}

#[cfg(test)]
mod tests {
    use crate::test_utils::{create_buffer_from, create_empty_buffer};

    #[test]
    fn ends_with_lf_returns_true_when_buffer_ends_with_lf() {
        let b = create_buffer_from("\n");
        assert!(b.ends_with_lf());
    }

    #[test]
    fn ends_with_lf_returns_false_when_buffer_is_empty() {
        let b = create_empty_buffer();
        assert!(!b.ends_with_lf());
    }

    #[test]
    fn ends_with_lf_returns_false_when_buffer_does_not_end_with_lf() {
        let b = create_buffer_from("hello");
        assert!(!b.ends_with_lf());
    }

    #[test]
    fn ends_with_lf_returns_false_when_buffer_ends_with_crlf() {
        let b = create_buffer_from("hello\r\n");
        assert!(!b.ends_with_lf());
    }

    #[test]
    fn ends_with_crlf_returns_true_when_buffer_ends_with_crlf() {
        let b = create_buffer_from("\r\n");
        assert!(b.ends_with_crlf());
    }

    #[test]
    fn ends_with_crlf_returns_false_when_buffer_does_not_end_with_crlf() {
        let b = create_buffer_from("hello");
        assert!(!b.ends_with_crlf());
    }

    #[test]
    fn ends_with_crlf_returns_false_when_buffer_is_empty() {
        let b = create_empty_buffer();
        assert!(!b.ends_with_crlf());
    }

    #[test]
    fn ends_with_crlf_returns_false_when_buffer_ends_with_lf() {
        let b = create_buffer_from("\n");
        assert!(!b.ends_with_crlf());
    }

    #[test]
    fn ends_with_crlf_returns_false_when_buffer_length_is_less_than_two() {
        let b = create_buffer_from("h");
        assert!(!b.ends_with_crlf());
    }

    #[test]
    fn ends_with_newline_returns_false_when_buffer_does_not_end_with_crlf_or_lf() {
        let b = create_buffer_from("a\r\nb");
        assert!(!b.ends_with_newline());
    }

    #[test]
    fn ends_with_newline_returns_false_for_trailing_cr_without_lf() {
        let b = create_buffer_from("hello\r");
        assert!(!b.ends_with_newline());
    }

    #[test]
    fn get_line_returns_empty_string_with_empty_buffer() {
        let b = create_empty_buffer();
        assert_eq!(b.get_line(0), String::new());
    }

    #[test]
    fn get_line_returns_empty_string_with_out_of_bounds_line_idx() {
        let b = create_empty_buffer();
        let idx = 9999;
        assert_eq!(b.get_line(idx), String::new());
    }

    #[test]
    fn get_line_returns_empty_string_when_ends_with_newline_and_last_line_requested() {
        let b = create_buffer_from("\n\n\n");
        let idx = 2;
        assert_eq!(b.get_line(idx), String::new());
    }

    #[test]
    fn get_text_range_returns_correct_slice_on_out_of_bounds_idx() {
        let b = create_buffer_from("hello");
        assert_eq!(b.get_text_range(0, 1000000), String::from("hello"));
    }

    #[test]
    fn get_text_range_returns_empty_string_on_empty_buffer() {
        let b = create_empty_buffer();
        assert_eq!(b.get_text_range(0, 1), String::new());
    }

    #[test]
    fn get_text_range_returns_empty_string_on_same_starting_and_ending_idx() {
        let b = create_empty_buffer();
        assert_eq!(b.get_text_range(0, 0), String::new());
    }

    #[test]
    fn get_text_range_returns_correct_slice_when_starting_idx_greater_than_ending_idx() {
        let b = create_buffer_from("hello");
        assert_eq!(b.get_text_range(5, 0), String::from("hello"));
    }

    #[test]
    fn visual_line_count_returns_one_when_buffer_is_empty() {
        let b = create_empty_buffer();
        assert_eq!(b.visual_line_count(), 1);
    }

    #[test]
    fn visual_line_count_returns_one_when_there_is_one_line() {
        let b = create_buffer_from("0");
        assert_eq!(b.visual_line_count(), 1);
    }

    #[test]
    fn visual_line_count_returns_two_when_first_line_has_content_and_ends_with_newline() {
        let b = create_buffer_from("a\n");
        assert_eq!(b.visual_line_count(), 2);
    }

    #[test]
    fn visual_line_count_returns_two_when_first_line_ends_with_newline_only() {
        let b = create_buffer_from("\n");
        assert_eq!(b.visual_line_count(), 2);
    }

    #[test]
    fn line_len_without_newline_returns_three_when_first_line_has_three_chars_and_ends_with_lf() {
        let b = create_buffer_from("abc\n");
        assert_eq!(b.line_len_without_newline(0), 3);
    }

    #[test]
    fn line_len_without_newline_returns_three_when_first_line_has_three_chars_and_ends_with_crlf() {
        let b = create_buffer_from("abc\r\n");
        assert_eq!(b.line_len_without_newline(0), 3);
    }

    #[test]
    fn line_len_without_newline_returns_three_when_first_line_has_three_chars() {
        let b = create_buffer_from("abc");
        assert_eq!(b.line_len_without_newline(0), 3);
    }

    #[test]
    fn line_len_without_newline_returns_zero_when_buffer_is_empty() {
        let b = create_empty_buffer();
        assert_eq!(b.line_len_without_newline(0), 0);
    }

    #[test]
    fn line_len_without_newline_returns_zero_when_buffer_has_only_lf() {
        let b = create_buffer_from("\n");
        assert_eq!(b.line_len_without_newline(0), 0);
    }

    #[test]
    fn line_len_without_newline_returns_zero_when_buffer_has_only_crlf() {
        let b = create_buffer_from("\r\n");
        assert_eq!(b.line_len_without_newline(0), 0);
    }

    #[test]
    fn byte_to_row_col_returns_zero_zero_on_zeroth_byte() {
        let b = create_buffer_from("ab\ncd");
        assert_eq!(b.byte_to_row_col(0), (0, 0));
    }

    #[test]
    fn byte_to_row_col_returns_zero_one_on_first_byte() {
        let b = create_buffer_from("ab\ncd");
        assert_eq!(b.byte_to_row_col(1), (0, 1));
    }

    #[test]
    fn byte_to_row_col_returns_zero_two_on_second_byte() {
        let b = create_buffer_from("ab\ncd");
        assert_eq!(b.byte_to_row_col(2), (0, 2));
    }

    #[test]
    fn byte_to_row_col_returns_one_zero_on_first_byte_after_newline() {
        let b = create_buffer_from("ab\ncd");
        assert_eq!(b.byte_to_row_col(3), (1, 0));
    }

    #[test]
    fn byte_to_row_col_returns_one_one_on_last_byte_in_buffer() {
        let b = create_buffer_from("ab\ncd");
        assert_eq!(b.byte_to_row_col(4), (1, 1));
    }

    #[test]
    fn byte_to_row_col_clamps_to_end_when_byte_at_or_past_len() {
        let b = create_buffer_from("ab\ncd");
        assert_eq!(b.byte_to_row_col(5), (1, 2));
    }

    #[test]
    fn byte_to_row_col_returns_next_line_start_at_eof_when_ends_with_newline() {
        let b = create_buffer_from("ab\ncd\n");
        assert_eq!(b.byte_to_row_col(6), (2, 0));
    }

    #[test]
    fn byte_to_row_col_crlf_maps_after_newline_to_next_line_start() {
        let b = create_buffer_from("ab\r\ncd");
        assert_eq!(b.byte_to_row_col(4), (1, 0));
    }

    #[test]
    fn insert_does_nothing_if_pos_greater_than_byte_len() {
        let mut b = create_empty_buffer();
        b.insert(100, "hello");
        assert_eq!(b.get_text(), String::new());
    }

    #[test]
    fn insert_inserts_text_when_pos_less_than_or_equal_to_byte_len() {
        let b = create_buffer_from("hello");
        assert_eq!(b.get_text(), "hello");
    }

    #[test]
    fn clear_empties_buffer() {
        let mut b = create_buffer_from("abcdefg");

        b.clear();
        assert_eq!(b.get_text(), String::new());
    }

    #[test]
    fn backspace_does_nothing_at_buffer_start() {
        let mut b = create_buffer_from("abc");
        b.backspace(0);

        assert_eq!(b.get_text(), "abc");
    }

    #[test]
    fn backspace_deletes_single_char_when_idx_greater_than_zero() {
        let mut b = create_buffer_from("abc");
        b.backspace(3);

        assert_eq!(b.get_text(), "ab");
    }

    #[test]
    fn backspace_deletes_single_char_on_empty_line_that_is_not_first_line() {
        let mut b = create_buffer_from("abc\n");
        b.backspace(4);

        assert_eq!(b.get_text(), "abc");
    }

    #[test]
    fn delete_at_does_nothing_in_empty_buffer() {
        let mut b = create_empty_buffer();
        b.delete_at(0);

        assert_eq!(b.get_text(), String::new());
    }

    #[test]
    fn delete_at_deletes_properly_in_line_middle() {
        let mut b = create_buffer_from("abc");
        b.delete_at(1);

        assert_eq!(b.get_text(), "ac");
    }

    #[test]
    fn delete_at_returns_deleted_slice() {
        let mut b = create_buffer_from("abc");
        let result = b.delete_at(0);

        assert_eq!(result, "a");
    }

    #[test]
    fn delete_at_returns_empty_string_if_idx_beyond_buffer_length() {
        let mut b = create_buffer_from("abcd");
        let result = b.delete_at(10);

        assert_eq!(result, String::new());
    }

    #[test]
    fn delete_range_does_nothing_in_empty_buffer() {
        let mut b = create_empty_buffer();
        b.delete_range(0, 1);

        assert_eq!(b.get_text(), String::new());
    }

    #[test]
    fn delete_range_does_nothing_when_idxes_out_of_bounds() {
        let mut b = create_buffer_from("abc");
        b.delete_range(100, 200);

        assert_eq!(b.get_text(), "abc");
    }

    #[test]
    fn delete_range_returns_empty_string_if_idxes_out_of_bounds() {
        let mut b = create_buffer_from("abc");
        let result = b.delete_range(100, 200);

        assert_eq!(result, String::new());
    }

    #[test]
    fn delete_range_returns_empty_string_if_idxes_are_equal() {
        let mut b = create_buffer_from("abc");
        let result = b.delete_range(1, 1);

        assert_eq!(result, String::new());
    }

    #[test]
    fn delete_range_does_nothing_if_idxes_are_equal() {
        let mut b = create_buffer_from("abc");
        b.delete_range(1, 1);

        assert_eq!(b.get_text(), "abc");
    }

    #[test]
    fn delete_range_handles_start_idx_greater_than_end_idx() {
        let mut b = create_buffer_from("abc");
        b.delete_range(1, 0);

        assert_eq!(b.get_text(), "bc");
    }

    #[test]
    fn delete_range_returns_correct_string_when_start_idx_greater_than_end_idx() {
        let mut b = create_buffer_from("abc");
        let result = b.delete_range(2, 0);

        assert_eq!(result, "ab");
    }

    #[test]
    fn delete_range_deletes_clamped_slice_when_start_out_of_bounds_but_end_in_bounds_and_start_at_end()
     {
        let mut b = create_buffer_from("abc");
        let deleted = b.delete_range(100, 2);

        assert_eq!(deleted, "c");
        assert_eq!(b.get_text(), "ab");
    }

    #[test]
    fn line_to_byte_and_byte_to_line_round_trip_basic() {
        let b = create_buffer_from("ab\ncd\nef");
        for line in 0..b.content.line_len() {
            let start = b.line_to_byte(line);
            assert_eq!(b.byte_to_line(start), line);
        }
    }

    #[test]
    fn line_to_byte_last_visual_line_points_to_eof_when_ends_with_newline() {
        let b = create_buffer_from("a\n");
        assert_eq!(b.line_to_byte(1), b.byte_len());
    }
}
