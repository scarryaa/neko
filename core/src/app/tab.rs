use crate::Editor;
use std::path::{Path, PathBuf};

#[derive(Debug, Default)]
pub struct Tab {
    editor: Editor,
    original_content: String,
    file_path: Option<PathBuf>,
    title: String,
}

impl Tab {
    pub fn new() -> Self {
        Self {
            editor: Editor::new(),
            original_content: String::new(),
            file_path: None,
            title: "Untitled".to_string(),
        }
    }

    pub fn with_title(title: &str) -> Self {
        Self {
            editor: Editor::new(),
            original_content: String::new(),
            file_path: None,
            title: title.to_string(),
        }
    }

    // Getters
    pub fn get_title(&self) -> String {
        self.title.clone()
    }

    pub fn get_file_path(&self) -> Option<&Path> {
        self.file_path.as_deref()
    }

    pub fn get_original_content(&self) -> String {
        self.original_content.clone()
    }

    pub fn get_editor(&self) -> &Editor {
        &self.editor
    }

    pub fn get_editor_mut(&mut self) -> &mut Editor {
        &mut self.editor
    }

    pub fn get_modified(&self) -> bool {
        self.editor.buffer().get_text() != self.original_content
    }

    // Setters
    pub fn set_title(&mut self, title: &str) {
        if title.is_empty() {
            self.title = "Untitled".to_string()
        } else {
            self.title = title.to_string();
        }
    }

    pub fn set_file_path(&mut self, file_path: Option<String>) {
        if let Some(file_path) = file_path {
            self.file_path = Some(file_path.into());
        } else {
            self.file_path = None;
        }
    }

    pub fn set_original_content(&mut self, new_content: String) {
        self.original_content = new_content;
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn get_modified_returns_false_if_buffer_and_original_content_are_empty() {
        let t = Tab::new();
        assert!(!t.get_modified());
    }

    #[test]
    fn get_modified_returns_true_if_buffer_does_not_equal_original_content() {
        let mut t = Tab::new();
        t.set_original_content("hello".to_string());
        assert!(t.get_modified());

        let mut t2 = Tab::new();
        t2.get_editor_mut().insert_text("hello");
        assert!(t.get_modified());
    }

    #[test]
    fn set_title_sets_title_to_untitled_if_title_empty() {
        let mut t = Tab::new();
        t.set_title("");

        assert_eq!(t.get_title(), "Untitled");
    }

    #[test]
    fn set_title_updates_title_correctly() {
        let mut t = Tab::new();
        t.set_title("my file");

        assert_eq!(t.get_title(), "my file");
    }
}
