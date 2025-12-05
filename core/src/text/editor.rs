use crate::{Buffer, Cursor, Selection};

#[derive(Default)]
pub struct Editor {
    buffer: Buffer,
    cursor: Cursor,
    selection: Selection,
}

impl Editor {
    pub fn new() -> Self {
        Self {
            buffer: Buffer::new(),
            cursor: Cursor::new(),
            selection: Selection::new(),
        }
    }

    pub fn buffer(&self) -> &Buffer {
        &self.buffer
    }

    pub fn cursor(&self) -> &Cursor {
        &self.cursor
    }

    pub fn selection(&self) -> &Selection {
        &self.selection
    }

    pub fn insert_text(&mut self, text: &str) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
        }

        self.cursor.insert_and_move(&mut self.buffer, text);
        self.selection.clear();
    }

    pub fn insert_newline(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
        }

        self.cursor.insert_newline(&mut self.buffer);
        self.selection.clear();
    }

    pub fn insert_tab(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
        }

        self.cursor.insert_tab(&mut self.buffer);
        self.selection.clear();
    }

    pub fn backspace(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
        } else {
            self.cursor.backspace_and_move(&mut self.buffer);
        }

        self.selection.clear();
    }

    pub fn delete(&mut self) {
        if self.selection.is_active() {
            self.cursor = self.selection.start().clone();
            self.selection.delete(&mut self.buffer);
        } else {
            self.cursor.delete_at_cursor(&mut self.buffer);
        }

        self.selection.clear();
    }

    pub fn move_left(&mut self) {
        self.cursor.move_left(&self.buffer);
        self.selection.clear();
    }

    pub fn move_right(&mut self) {
        self.cursor.move_right(&self.buffer);
        self.selection.clear();
    }

    pub fn move_up(&mut self) {
        self.cursor.move_up(&self.buffer);
        self.selection.clear();
    }

    pub fn move_down(&mut self) {
        self.cursor.move_down(&self.buffer);
        self.selection.clear();
    }

    pub fn select_left(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_left(&self.buffer);
        self.selection.update(&self.cursor, &self.buffer);
    }

    pub fn select_right(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_right(&self.buffer);
        self.selection.update(&self.cursor, &self.buffer);
    }

    pub fn select_up(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_up(&self.buffer);
        self.selection.update(&self.cursor, &self.buffer);
    }

    pub fn select_down(&mut self) {
        if !self.selection.is_active() {
            self.selection.begin(&self.cursor);
        }

        self.cursor.move_down(&self.buffer);
        self.selection.update(&self.cursor, &self.buffer);
    }

    pub fn select_all(&mut self) {
        self.selection.begin(&Cursor::new());
        self.cursor.move_to_end(&self.buffer);
        self.selection.update(&self.cursor, &self.buffer);
    }

    pub fn clear_selection(&mut self) {
        self.selection.clear();
    }
}
