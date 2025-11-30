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
}
