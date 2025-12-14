use super::{Buffer, Cursor};

#[derive(Debug, Default, PartialEq, Eq, Clone)]
pub struct Selection {
    pub(crate) start: Cursor,
    pub(crate) end: Cursor,
    pub(crate) anchor: Cursor,
    active: bool,
}

impl Selection {
    pub fn new() -> Self {
        Self {
            start: Cursor::new(),
            end: Cursor::new(),
            anchor: Cursor::new(),
            active: false,
        }
    }

    pub fn is_active(&self) -> bool {
        self.active && self.start != self.end
    }

    pub fn begin(&mut self, cursor: &Cursor) {
        self.anchor = cursor.clone();
        self.start = cursor.clone();
        self.end = cursor.clone();
        self.active = true;
    }

    pub fn update(&mut self, cursor: &Cursor, buffer: &Buffer) {
        if !self.active {
            return;
        }

        let anchor_idx = self.anchor.get_idx(buffer);
        let cursor_idx = cursor.get_idx(buffer);

        if cursor_idx < anchor_idx {
            self.start = cursor.clone();
            self.end = self.anchor.clone();
        } else {
            self.start = self.anchor.clone();
            self.end = cursor.clone();
        }
    }

    pub fn clear(&mut self) {
        self.active = false;
        self.start = Cursor::new();
        self.end = Cursor::new();
    }
}
