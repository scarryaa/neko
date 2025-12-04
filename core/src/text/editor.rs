use crate::{Buffer, Cursor};

pub struct Editor {
    buffer: Buffer,
    cursor: Cursor,
}

impl Editor {
    pub fn new() -> Self {
        Self {
            buffer: Buffer::new(),
            cursor: Cursor::new(),
        }
    }

    pub fn buffer(&self) -> &Buffer {
        &self.buffer
    }

    pub fn cursor(&self) -> &Cursor {
        &self.cursor
    }

    pub fn insert_text(&mut self, text: &str) {
        self.cursor.insert_and_move(&mut self.buffer, text);
    }

    pub fn insert_newline(&mut self) {
        self.cursor.insert_newline(&mut self.buffer);
    }

    pub fn insert_tab(&mut self) {
        self.cursor.insert_tab(&mut self.buffer);
    }

    pub fn backspace(&mut self) {
        self.cursor.backspace_and_move(&mut self.buffer);
    }

    pub fn delete(&mut self) {
        self.cursor.delete_at_cursor(&mut self.buffer);
    }

    pub fn move_left(&mut self) {
        self.cursor.move_left(&self.buffer);
    }

    pub fn move_right(&mut self) {
        self.cursor.move_right(&self.buffer);
    }

    pub fn move_up(&mut self) {
        self.cursor.move_up(&self.buffer);
    }

    pub fn move_down(&mut self) {
        self.cursor.move_down(&self.buffer);
    }
}
