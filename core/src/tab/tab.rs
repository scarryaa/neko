use crate::DocumentId;

#[derive(Debug)]
pub struct Tab {
    id: usize,
    document_id: DocumentId,
    scroll_offsets: (i32, i32),
    is_pinned: bool,
}

impl Tab {
    pub fn new(id: usize, document_id: DocumentId) -> Self {
        Self {
            document_id,
            is_pinned: false,
            id,
            scroll_offsets: (0, 0),
        }
    }

    // Getters
    pub fn get_id(&self) -> usize {
        self.id
    }

    pub fn get_document_id(&self) -> DocumentId {
        self.document_id
    }

    pub fn get_is_pinned(&self) -> bool {
        self.is_pinned
    }

    pub fn get_scroll_offsets(&self) -> (i32, i32) {
        self.scroll_offsets
    }

    // Setters
    pub fn set_is_pinned(&mut self, new_is_pinned: bool) {
        self.is_pinned = new_is_pinned;
    }

    pub fn set_scroll_offsets(&mut self, new_offsets: (i32, i32)) {
        self.scroll_offsets = new_offsets;
    }
}

#[cfg(test)]
mod test {
    use crate::Buffer;

    use super::*;

    #[test]
    fn get_modified_returns_false_if_buffer_and_original_content_are_empty() {
        let t = Tab::new(0);
        let b = Buffer::new();
        assert!(!t.get_modified(&b));
    }

    #[test]
    fn get_modified_returns_true_if_buffer_does_not_equal_original_content() {
        let mut t = Tab::new(0);
        let b = Buffer::new();
        t.set_original_content("hello".to_string());
        assert!(t.get_modified(&b));

        let mut t2 = Tab::new(1);
        let mut b = Buffer::new();
        t2.get_editor_mut().insert_text(&mut b, "hello");
        assert!(t.get_modified(&b));
    }

    #[test]
    fn set_title_sets_title_to_untitled_if_title_empty() {
        let mut t = Tab::new(0);
        t.set_title("");

        assert_eq!(t.get_title(), "Untitled");
    }

    #[test]
    fn set_title_updates_title_correctly() {
        let mut t = Tab::new(0);
        t.set_title("my file");

        assert_eq!(t.get_title(), "my file");
    }
}
