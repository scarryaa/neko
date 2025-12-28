use super::{Buffer, Cursor, Selection};

#[derive(Default, Debug)]
pub struct SelectionManager {
    // TODO(scarlet): Implement multi-selection (one selection per cursor)
    pub(crate) selection: Selection,
}

impl SelectionManager {
    pub fn new() -> Self {
        Self {
            selection: Selection::new(),
        }
    }

    pub fn selection(&self) -> &Selection {
        &self.selection
    }

    pub fn has_active_selection(&self) -> bool {
        self.selection.is_active()
    }

    pub fn reset_selection(&mut self) {
        self.selection = Selection::new();
    }

    pub fn clear_selection(&mut self) {
        self.selection.clear();
    }

    pub fn ensure_begin(&mut self, cursor: &Cursor) {
        if !self.selection.is_active() {
            self.selection.begin(cursor);
        }
    }

    pub fn update(&mut self, cursor: &Cursor, buffer: &Buffer) {
        self.selection.update(cursor, buffer);
    }

    pub fn set_selection(&mut self, selection: &Selection) {
        self.selection = selection.clone();
    }
}
