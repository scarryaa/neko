use crop::Rope;

#[derive(Debug, Default)]
pub struct Buffer {
    content: Rope,
}

impl Buffer {
    pub fn new() -> Self {
        Self {
            content: Rope::from("\n"),
        }
    }

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
        if pos < self.content.byte_len() && self.content.byte_len() != 1 {
            self.content.delete(pos..pos + 1);
            deleted
        } else {
            "".to_string()
        }
    }

    pub fn delete_range(&mut self, start_pos: usize, end_pos: usize) -> String {
        let deleted = self.get_text_range(start_pos, end_pos);

        if start_pos < self.content.byte_len() && end_pos <= self.content.byte_len() {
            self.content.delete(start_pos..end_pos);
        }

        deleted
    }

    pub fn get_text(&self) -> String {
        self.content.to_string()
    }

    pub fn get_text_range(&self, start_idx: usize, end_idx: usize) -> String {
        self.content.byte_slice(start_idx..end_idx).to_string()
    }

    pub fn get_line(&self, line_idx: usize) -> String {
        if line_idx >= self.content.line_len() {
            return "".to_string();
        }

        self.content.line(line_idx).to_string()
    }

    pub fn line_count(&self) -> usize {
        self.content.line_len()
    }

    pub fn line_len(&self, line_idx: usize) -> usize {
        self.content.line(line_idx).byte_len()
    }

    pub fn byte_len(&self) -> usize {
        self.content.byte_len()
    }

    pub fn line_to_byte(&self, line_idx: usize) -> usize {
        self.content.byte_of_line(line_idx)
    }

    pub fn byte_to_line(&self, byte_idx: usize) -> usize {
        self.content.line_of_byte(byte_idx)
    }

    pub fn byte_to_row_col(&self, byte_idx: usize) -> (usize, usize) {
        let row = self.byte_to_line(byte_idx);
        let line_start = self.line_to_byte(row);
        let col = byte_idx.saturating_sub(line_start);

        let col = col.min(self.line_len_without_newline(row));
        (row, col)
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
}
