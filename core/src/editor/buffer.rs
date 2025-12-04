use crop::Rope;

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

    pub fn insert(&mut self, pos: usize, text: &str) {
        self.content.insert(pos, text);
    }

    pub fn get_text(&self) -> String {
        self.content.to_string()
    }

    pub fn line_count(&self) -> usize {
        self.content.line_len()
    }

    pub fn line_len(&self, line_idx: usize) -> usize {
        self.content.line(line_idx).byte_len()
    }
}
