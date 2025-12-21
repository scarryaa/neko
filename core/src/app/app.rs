use super::Tab;
use crate::{Editor, FileTree};
use std::{
    io::{Error, ErrorKind},
    path::{Path, PathBuf},
};

#[derive(Debug)]
pub struct AppState {
    tabs: Vec<Tab>,
    active_tab_index: usize,
    file_tree: FileTree,
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

    // Getters
    pub fn get_tab_count(&self) -> usize {
        self.tabs.len()
    }

    pub fn get_tabs_empty(&self) -> bool {
        self.tabs.is_empty()
    }

    pub fn get_active_editor(&self) -> Option<&Editor> {
        self.tabs.get(self.active_tab_index).map(|t| t.get_editor())
    }

    pub fn get_active_editor_mut(&mut self) -> Option<&mut Editor> {
        self.tabs
            .get_mut(self.active_tab_index)
            .map(|t| t.get_editor_mut())
    }

    pub fn get_active_tab_path(&self) -> Option<&str> {
        self.tabs
            .get(self.active_tab_index)
            .and_then(|t| t.get_file_path())
            .and_then(|p| p.to_str())
    }

    pub fn get_active_tab_index(&self) -> usize {
        self.active_tab_index
    }

    pub fn get_tab_titles(&self) -> Vec<String> {
        self.tabs.iter().map(|t| t.get_title().clone()).collect()
    }

    pub fn get_tab_modified_states(&self) -> Vec<bool> {
        self.tabs
            .iter()
            .map(|t| {
                let original_content = t.get_original_content();
                original_content != t.get_editor().buffer().get_text()
            })
            .collect()
    }

    pub fn get_tab_modified(&self, index: usize) -> bool {
        self.tabs
            .get(index)
            .map(|t| t.get_modified())
            .unwrap_or(false)
    }

    pub fn get_tab_pinned_states(&self) -> Vec<bool> {
        self.tabs.iter().map(|t| t.get_is_pinned()).collect()
    }

    pub fn get_tab_index_by_path(&self, path: &str) -> Option<usize> {
        self.tabs
            .iter()
            .position(|t| t.get_file_path().as_ref().and_then(|p| p.to_str()) == Some(path))
    }

    pub fn tab_with_path_exists(&self, path: &str) -> bool {
        self.tabs
            .iter()
            .any(|t| t.get_file_path().as_ref().and_then(|p| p.to_str()) == Some(path))
    }

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
    }

    pub fn get_tab_path(&self, index: usize) -> Option<&str> {
        self.tabs.get(index).and_then(|t| {
            if let Some(path) = t.get_file_path() {
                path.to_str()
            } else {
                Some("")
            }
        })
    }

    // Setters
    pub fn new_tab(&mut self) -> usize {
        let tab = Tab::new();

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

    pub fn close_other_tabs(&mut self, index: usize) -> Result<(), Error> {
        if index < self.tabs.len() {
            for i in (0..self.tabs.len()).rev() {
                if i == index {
                    continue;
                }

                self.tabs.remove(i);
            }

            if !self.tabs.is_empty() {
                self.active_tab_index -= self.tabs.len().saturating_sub(1);
            }

            Ok(())
        } else {
            Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"))
        }
    }

    pub fn close_left_tabs(&mut self, index: usize) -> Result<(), Error> {
        if index < self.tabs.len() {
            for i in (0..index).rev() {
                self.tabs.remove(i);
            }

            if !self.tabs.is_empty() {
                self.active_tab_index -= index;
            }

            Ok(())
        } else {
            Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"))
        }
    }

    pub fn close_right_tabs(&mut self, index: usize) -> Result<(), Error> {
        if index < self.tabs.len() {
            for i in (index..self.tabs.len()).rev() {
                self.tabs.remove(i);
            }

            if !self.tabs.is_empty() {
                self.active_tab_index -= self.tabs.len().saturating_sub(1) - index;
            }

            Ok(())
        } else {
            Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"))
        }
    }

    pub fn pin_tab(&mut self, index: usize) -> Result<(), Error> {
        if index < self.tabs.len() {
            if !self.tabs.is_empty() {
                if let Some(t) = self.tabs.get_mut(index) {
                    t.set_is_pinned(true)
                }
            }

            self.reorder_tabs_by_pin();
            Ok(())
        } else {
            Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"))
        }
    }

    pub fn unpin_tab(&mut self, index: usize) -> Result<(), Error> {
        if index < self.tabs.len() {
            if !self.tabs.is_empty() {
                if let Some(t) = self.tabs.get_mut(index) {
                    t.set_is_pinned(false)
                }
            }

            self.reorder_tabs_by_pin();
            Ok(())
        } else {
            Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"))
        }
    }

    pub fn set_active_tab_index(&mut self, index: usize) -> Result<(), Error> {
        if self.tabs.is_empty() || index >= self.tabs.len() {
            Err(Error::other("Provided tab index is out of range"))
        } else {
            self.active_tab_index = index;
            Ok(())
        }
    }

    pub fn open_file(&mut self, path: &str) -> Result<(), std::io::Error> {
        // Check if file is already open
        if self
            .tabs
            .iter()
            .any(|t| t.get_file_path().as_ref().and_then(|p| p.to_str()) == Some(path))
        {
            return Err(Error::new(
                ErrorKind::AlreadyExists,
                "File with given path already open",
            ));
        }

        let content = std::fs::read_to_string(path)?;

        if let Some(t) = self.tabs.get_mut(self.active_tab_index) {
            t.get_editor_mut().load_file(&content);
            t.set_original_content(content);
            t.set_file_path(Some(path.into()));
            t.set_title(
                Path::new(path)
                    .file_name()
                    .and_then(|n| n.to_str())
                    .unwrap_or(path),
            );
        }

        Ok(())
    }

    pub fn save_active_tab(&mut self) -> Result<(), std::io::Error> {
        let tab = self
            .tabs
            .get_mut(self.active_tab_index)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No active tab"))?;

        let maybe_file_path = tab.get_file_path();
        let path = maybe_file_path
            .as_ref()
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No current file"))?;

        let content = tab.get_editor().buffer().get_text();
        std::fs::write(path, &content)?;

        tab.set_original_content(content.clone());

        Ok(())
    }

    pub fn save_active_tab_and_set_path(&mut self, path: &str) -> Result<(), std::io::Error> {
        if path.is_empty() {
            return Err(Error::new(ErrorKind::InvalidInput, "Path cannot be empty"));
        }

        let tab = self
            .tabs
            .get_mut(self.active_tab_index)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "No active tab"))?;

        let content = tab.get_editor().buffer().get_text();
        tab.set_original_content(content.clone());
        std::fs::write(path, content)?;
        tab.set_file_path(Some(path.to_string()));

        if let Some(filename) = PathBuf::from(path).file_name() {
            tab.set_title(&filename.to_string_lossy());
        }

        Ok(())
    }

    fn reorder_tabs_by_pin(&mut self) {
        if self.tabs.is_empty() {
            self.active_tab_index = 0;
            return;
        }

        let active_index = self.active_tab_index;
        let mut pinned: Vec<(usize, Tab)> = Vec::new();
        let mut unpinned: Vec<(usize, Tab)> = Vec::new();

        for (i, tab) in self.tabs.drain(..).enumerate() {
            if tab.get_is_pinned() {
                pinned.push((i, tab));
            } else {
                unpinned.push((i, tab));
            }
        }

        let mut new_tabs: Vec<Tab> = Vec::with_capacity(pinned.len() + unpinned.len());
        let mut new_active_index = 0;

        for (new_index, (old_index, tab)) in
            pinned.into_iter().chain(unpinned.into_iter()).enumerate()
        {
            if old_index == active_index {
                new_active_index = new_index;
            }

            new_tabs.push(tab);
        }

        self.tabs = new_tabs;
        self.active_tab_index = new_active_index;
    }
}

#[cfg(test)]
mod test {
    use super::*;

    #[test]
    fn close_tab_returns_error_when_tabs_are_empty() {
        let mut a = AppState::new(None).unwrap();
        let result = a.close_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn close_tab_returns_error_when_index_greater_than_number_of_tabs() {
        let mut a = AppState::new(None).unwrap();
        let result = a.close_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_index_returns_error_when_tabs_are_empty() {
        let mut a = AppState::new(None).unwrap();
        let _ = a.close_tab(0);
        let result = a.set_active_tab_index(0);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_index_returns_error_when_index_greater_than_number_of_tabs() {
        let mut a = AppState::new(None).unwrap();
        let result = a.set_active_tab_index(1);

        assert!(result.is_err())
    }

    #[test]
    fn open_file_returns_error_when_file_is_already_open() {
        let mut a = AppState::new(None).unwrap();
        a.new_tab();
        a.tabs[1].set_file_path(Some("test".to_string()));

        let result = a.open_file("test");

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_returns_error_when_tabs_are_empty() {
        let mut a = AppState::new(None).unwrap();
        let _ = a.close_tab(0);
        let result = a.save_active_tab();

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_returns_error_when_there_is_no_current_file() {
        let mut a = AppState::new(None).unwrap();
        let result = a.save_active_tab();

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_and_set_path_returns_error_when_provided_path_is_empty() {
        let mut a = AppState::new(None).unwrap();
        let result = a.save_active_tab_and_set_path("");

        assert!(result.is_err())
    }

    #[test]
    fn save_active_tab_and_set_path_returns_error_when_tabs_are_empty() {
        let mut a = AppState::new(None).unwrap();
        let _ = a.close_tab(0);
        let result = a.save_active_tab_and_set_path("");

        assert!(result.is_err())
    }

    #[test]
    fn pin_tab_reorders_tabs_and_updates_active_index() {
        let mut a = AppState::new(None).unwrap();
        a.new_tab();
        a.new_tab();

        a.tabs[0].set_title("A");
        a.tabs[1].set_title("B");
        a.tabs[2].set_title("C");

        let _ = a.set_active_tab_index(2);
        a.pin_tab(2).unwrap();

        assert_eq!(a.get_tab_titles(), vec!["C", "A", "B"]);
        assert_eq!(a.get_active_tab_index(), 0);
    }
}
