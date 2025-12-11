use std::{
    io::{Error, ErrorKind},
    path::{Path, PathBuf},
};

use crate::{FileTree, text::Editor};

pub struct AppState {
    tabs: Vec<Tab>,
    active_tab_index: usize,
    file_tree: FileTree,
}

pub struct Tab {
    editor: Editor,
    original_content: Option<String>,
    file_path: Option<PathBuf>,
    title: String,
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
            original_content: Some("\n".to_owned()),
            file_path: None,
            title: "Untitled".to_string(),
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

    pub fn get_tab_count(&self) -> usize {
        self.tabs.len()
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

    pub fn get_tab_modified_states(&self) -> Vec<bool> {
        self.tabs
            .iter()
            .map(|t| {
                t.original_content.clone().unwrap_or("".to_string()) != t.editor.buffer().get_text()
            })
            .collect()
    }

    pub fn get_tab_modified(&self, index: usize) -> bool {
        self.tabs
            .get(index)
            .map(|t| {
                t.original_content.clone().unwrap_or("".to_string()) != t.editor.buffer().get_text()
            })
            .unwrap()
    }

    pub fn tab_with_path_exists(&self, path: &str) -> bool {
        self.tabs
            .iter()
            .any(|t| t.file_path.as_ref().and_then(|p| p.to_str()) == Some(path))
    }

    pub fn get_tab_index_by_path(&self, path: &str) -> Option<usize> {
        self.tabs
            .iter()
            .position(|t| t.file_path.as_ref().and_then(|p| p.to_str()) == Some(path))
    }

    pub fn open_file(&mut self, path: &str) -> Result<(), std::io::Error> {
        // Check if file is already open
        if self
            .tabs
            .iter()
            .any(|t| t.file_path.as_ref().and_then(|p| p.to_str()) == Some(path))
        {
            return Err(Error::new(
                ErrorKind::AlreadyExists,
                "File with given path already open",
            ));
        }

        let content = std::fs::read_to_string(path)?;

        if let Some(t) = self.tabs.get_mut(self.active_tab_index) {
            t.editor.load_file(&content);
            t.original_content = Some(content);
            t.file_path = Some(path.into());

            t.title = Path::new(path)
                .file_name()
                .and_then(|n| n.to_str())
                .unwrap_or(path)
                .to_string();
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
        tab.original_content = Some(content.clone());
        std::fs::write(path, content)?;
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
        tab.original_content = Some(content.clone());
        std::fs::write(path, content)?;
        tab.file_path = Some(PathBuf::from(path));

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
