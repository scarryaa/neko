use std::{
    io::{Error, ErrorKind},
    path::PathBuf,
};

use crate::{FileTree, text::Editor};

pub struct AppState {
    tabs: Vec<Tab>,
    active_tab_index: usize,
    file_tree: FileTree,
}

pub struct Tab {
    editor: Editor,
    file_path: Option<PathBuf>,
    title: String,
    is_modified: bool,
}

impl AppState {
    pub fn new(root_path: Option<&str>) -> Result<Self, std::io::Error> {
        let mut state = Self {
            tabs: Vec::new(),
            active_tab_index: 0,
            file_tree: FileTree::new(root_path)?,
        };

        state.new_tab();

        Ok(state)
    }

    pub fn new_tab(&mut self) -> usize {
        let tab = Tab {
            editor: Editor::new(),
            file_path: None,
            title: "Untitled".to_string(),
            is_modified: false,
        };
        self.tabs.push(tab);
        self.active_tab_index = self.tabs.len() - 1;
        self.active_tab_index
    }

    pub fn close_tab(&mut self, index: usize) -> Result<(), Error> {
        if index < self.tabs.len() {
            self.tabs.remove(index);
            if self.active_tab_index >= self.tabs.len() && !self.tabs.is_empty() {
                self.active_tab_index = self.tabs.len() - 1;
            }
            Ok(())
        } else {
            Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"))
        }
    }

    pub fn get_active_editor(&self) -> Option<&Editor> {
        self.tabs.get(self.active_tab_index).map(|t| &t.editor)
    }

    pub fn get_active_editor_mut(&mut self) -> Option<&mut Editor> {
        self.tabs
            .get_mut(self.active_tab_index)
            .map(|t| &mut t.editor)
    }

    pub fn get_active_tab_path(&self) -> Option<&str> {
        self.tabs
            .get(self.active_tab_index)
            .and_then(|t| t.file_path.as_ref())
            .and_then(|p| p.to_str())
    }

    pub fn get_active_tab_index(&self) -> usize {
        self.active_tab_index
    }

    pub fn get_tab_titles(&self) -> Vec<String> {
        self.tabs.iter().map(|t| t.title.clone()).collect()
    }

    pub fn open_file(&mut self, path: &str) -> Result<(), std::io::Error> {
        let content = std::fs::read_to_string(path)?;
        if let Some(t) = self.tabs.get_mut(self.active_tab_index) {
            t.editor.load_file(&content);
            t.file_path = Some(path.into());
            t.title = path.split('/').next_back().unwrap().to_string();
        }
        Ok(())
    }

    pub fn save_file(&mut self) -> Result<(), std::io::Error> {
        let tab = self
            .tabs
            .get_mut(self.active_tab_index)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No active tab"))?;

        let path = tab
            .file_path
            .as_ref()
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No current file"))?;

        let content = tab.editor.buffer().get_text();
        std::fs::write(path, content)?;
        tab.is_modified = false;
        Ok(())
    }

    pub fn save_and_set_path(&mut self, path: &str) -> Result<(), std::io::Error> {
        if path.is_empty() {
            return Err(Error::new(ErrorKind::InvalidInput, "Path cannot be empty"));
        }

        let tab = self
            .tabs
            .get_mut(self.active_tab_index)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No active tab"))?;

        let content = tab.editor.buffer().get_text();
        std::fs::write(path, content)?;
        tab.file_path = Some(PathBuf::from(path));
        tab.is_modified = false;

        if let Some(filename) = PathBuf::from(path).file_name() {
            tab.title = filename.to_string_lossy().to_string();
        }

        Ok(())
    }

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
    }

    pub fn set_active_tab_index(&mut self, index: usize) {
        self.active_tab_index = index;
    }
}
