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
        self.content.insert(pos, text);
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

    pub fn delete_at(&mut self, pos: usize) {
        if pos < self.content.byte_len() {
            self.content.delete(pos..pos + 1);
        }
    }

    pub fn delete_range(&mut self, start_pos: usize, end_pos: usize) {
        if start_pos < self.content.byte_len() && end_pos <= self.content.byte_len() {
            self.content.delete(start_pos..end_pos);
        }
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
}
