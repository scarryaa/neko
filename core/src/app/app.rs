use std::{
    io::{Error, ErrorKind},
    path::PathBuf,
};

use crate::{FileTree, text::Editor};

pub struct AppState {
    editor: Editor,
    file_tree: FileTree,
    current_file_path: Option<PathBuf>,
}

impl AppState {
    pub fn new(root_path: Option<&str>) -> Result<Self, std::io::Error> {
        Ok(Self {
            editor: Editor::new(),
            file_tree: FileTree::new(root_path)?,
            current_file_path: None,
        })
    }

    pub fn open_file(&mut self, path: &str) -> Result<(), std::io::Error> {
        let content = std::fs::read_to_string(path)?;
        self.editor.load_file(&content);
        self.current_file_path = Some(PathBuf::from(path));
        Ok(())
    }

    pub fn save_file(&mut self) -> Result<(), std::io::Error> {
        if let Some(path) = &self.current_file_path {
            let content = self.editor.buffer().get_text();
            std::fs::write(path, content)?;
            Ok(())
        } else {
            Err(Error::new(ErrorKind::NotFound, "No current file"))
        }
    }

    pub fn save_and_set_path(&mut self, path: &str) -> Result<(), std::io::Error> {
        if !path.is_empty() {
            let content = self.editor.buffer().get_text();
            std::fs::write(path, content)?;
            self.current_file_path = Some(path.into());
            Ok(())
        } else {
            Err(Error::new(ErrorKind::NotFound, "Save as failed"))
        }
    }

    pub fn get_editor(&self) -> &Editor {
        &self.editor
    }

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_editor_mut(&mut self) -> &mut Editor {
        &mut self.editor
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
    }

    pub fn get_current_file_path(&self) -> Option<&str> {
        self.current_file_path
            .as_ref()
            .map(|p| p.to_str())
            .flatten()
    }
}
