use super::Tab;
use crate::{Editor, FileTree};
use std::{
    io::{Error, ErrorKind},
    path::{Path, PathBuf},
};

#[derive(Debug)]
pub struct AppState {
    tabs: Vec<Tab>,
    active_tab_id: usize,
    file_tree: FileTree,
    next_tab_id: usize,
}

impl AppState {
    pub fn new(root_path: Option<&str>) -> Result<Self, std::io::Error> {
        let mut state = Self {
            tabs: Vec::new(),
            active_tab_id: 0,
            file_tree: FileTree::new(root_path)?,
            next_tab_id: 0,
        };

        state.new_tab();
        Ok(state)
    }

    // Getters
    pub fn get_tabs(&self) -> &Vec<Tab> {
        &self.tabs
    }

    pub fn get_active_tab_id(&self) -> usize {
        self.active_tab_id
    }

    pub fn get_active_editor(&self) -> Option<&Editor> {
        if let Some(index) = self
            .tabs
            .iter()
            .position(|t| t.get_id() == self.active_tab_id)
        {
            self.tabs.get(index).map(|t| t.get_editor())
        } else {
            None
        }
    }

    pub fn get_active_editor_mut(&mut self) -> Option<&mut Editor> {
        if let Some(index) = self
            .tabs
            .iter()
            .position(|t| t.get_id() == self.active_tab_id)
        {
            self.tabs.get_mut(index).map(|t| t.get_editor_mut())
        } else {
            None
        }
    }

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
    }

    pub fn get_close_other_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        if !self.tabs.iter().any(|t| t.get_id() == id) {
            return Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ));
        }

        Ok(self
            .tabs
            .iter()
            .filter(|t| t.get_id() != id && !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_all_tab_ids(&self) -> Result<Vec<usize>, Error> {
        Ok(self
            .tabs
            .iter()
            .filter(|t| !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_left_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        Ok(self.tabs[..index]
            .iter()
            .filter(|t| !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    pub fn get_close_right_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        Ok(self.tabs[index + 1..]
            .iter()
            .filter(|t| !t.get_is_pinned())
            .map(|t| t.get_id())
            .collect())
    }

    // Setters
    pub fn new_tab(&mut self) -> usize {
        let id = self.get_next_tab_id();
        let tab = Tab::new(id);

        self.tabs.push(tab);
        self.active_tab_id = id;
        id
    }

    pub fn close_tab(&mut self, id: usize) -> Result<(), Error> {
        if let Some(index) = self.tabs.iter().position(|t| t.get_id() == id) {
            self.tabs.remove(index);

            if self.active_tab_id == id && !self.tabs.is_empty() {
                if let Some(last_id) = self.tabs.last().map(|t| t.get_id()) {
                    self.active_tab_id = last_id;
                }
            } else if self.tabs.is_empty() {
                self.active_tab_id = 0;
            }

            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn close_other_tabs(&mut self, id: usize) -> Result<(), Error> {
        let exists = self.tabs.iter().any(|t| t.get_id() == id);
        if !exists {
            return Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ));
        }

        self.tabs.retain(|t| t.get_is_pinned() || t.get_id() == id);

        if self.tabs.is_empty() {
            self.active_tab_id = 0;
        } else {
            self.active_tab_id = id;
        }

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(())
    }

    pub fn close_left_tabs(&mut self, id: usize) -> Result<(), Error> {
        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        let mut i = 0usize;
        self.tabs.retain(|t| {
            let keep = i >= index || t.get_is_pinned();
            i += 1;
            keep
        });

        if !self.tabs.iter().any(|t| t.get_id() == self.active_tab_id) {
            self.active_tab_id = if self.tabs.is_empty() { 0 } else { id };
        }
        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(())
    }

    pub fn close_right_tabs(&mut self, id: usize) -> Result<(), Error> {
        let index = self
            .tabs
            .iter()
            .position(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        let mut i = 0usize;
        self.tabs.retain(|t| {
            let keep = i <= index || t.get_is_pinned();
            i += 1;
            keep
        });

        if !self.tabs.iter().any(|t| t.get_id() == self.active_tab_id) {
            self.active_tab_id = if self.tabs.is_empty() { 0 } else { id };
        }

        self.tabs.sort_by_key(|t| !t.get_is_pinned());

        Ok(())
    }

    pub fn close_all_tabs(&mut self) -> Result<(), Error> {
        // Remove all non-pinned tabs, keeping order of pinned ones
        self.tabs.retain(|t| t.get_is_pinned());

        // If the current active tab is gone, pick a new one
        let still_has_active = self.tabs.iter().any(|t| t.get_id() == self.active_tab_id);

        if !still_has_active {
            self.active_tab_id = self.tabs.first().map(|t| t.get_id()).unwrap_or(0);
        }

        Ok(())
    }

    pub fn pin_tab(&mut self, id: usize) -> Result<(), Error> {
        if let Some(index) = self.tabs.iter().position(|t| t.get_id() == id) {
            if !self.tabs.is_empty() {
                if let Some(t) = self.tabs.get_mut(index) {
                    t.set_is_pinned(true)
                }
            }

            self.reorder_tabs_by_pin();
            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn unpin_tab(&mut self, id: usize) -> Result<(), Error> {
        if let Some(index) = self.tabs.iter().position(|t| t.get_id() == id) {
            if !self.tabs.is_empty() {
                if let Some(t) = self.tabs.get_mut(index) {
                    t.set_is_pinned(false)
                }
            }

            self.reorder_tabs_by_pin();
            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn set_tab_scroll_offsets(
        &mut self,
        id: usize,
        new_offsets: (i32, i32),
    ) -> Result<(), Error> {
        if let Some(tab) = self.tabs.iter_mut().find(|t| t.get_id() == id) {
            tab.set_scroll_offsets(new_offsets);
            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn set_active_tab(&mut self, id: usize) -> Result<(), Error> {
        if self.tabs.iter().any(|t| t.get_id() == id) {
            self.active_tab_id = id;
            Ok(())
        } else {
            Err(Error::new(
                ErrorKind::NotFound,
                "Tab with given id not found",
            ))
        }
    }

    pub fn move_tab(&mut self, from: usize, to: usize) -> Result<(), Error> {
        let tab_count = self.tabs.len();
        if from >= tab_count || to > tab_count {
            return Err(Error::new(ErrorKind::InvalidInput, "Invalid tab index"));
        }

        if from == to || tab_count <= 1 {
            return Ok(());
        }

        let tab = self.tabs.remove(from);
        let mut insert_index = to;

        if from < insert_index {
            insert_index = insert_index.saturating_sub(1);
        }

        if insert_index > self.tabs.len() {
            insert_index = self.tabs.len();
        }

        self.tabs.insert(insert_index, tab);

        Ok(())
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

        if let Some(t) = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == self.active_tab_id)
        {
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
            .iter_mut()
            .find(|t| t.get_id() == self.active_tab_id)
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
            .iter_mut()
            .find(|t| t.get_id() == self.active_tab_id)
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

    pub fn save_tab_with_id(&mut self, id: usize) -> Result<(), Error> {
        let tab = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

        let path: PathBuf = tab
            .get_file_path()
            .ok_or_else(|| Error::new(ErrorKind::InvalidInput, "Tab path is missing"))?
            .to_path_buf();

        let content = tab.get_editor().buffer().get_text();

        std::fs::write(&path, &content)?;

        tab.set_original_content(content);

        if let Some(filename) = path.file_name() {
            tab.set_title(&filename.to_string_lossy());
        }

        Ok(())
    }

    pub fn save_tab_with_id_and_set_path(&mut self, id: usize, path: &str) -> Result<(), Error> {
        let tab = self
            .tabs
            .iter_mut()
            .find(|t| t.get_id() == id)
            .ok_or_else(|| Error::new(ErrorKind::NotFound, "Tab with given id not found"))?;

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
            self.active_tab_id = 0;
            return;
        }

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

        for (_, tab) in pinned.into_iter().chain(unpinned.into_iter()) {
            new_tabs.push(tab);
        }

        self.tabs = new_tabs;
    }

    pub fn reveal_in_file_tree(&mut self, path: &str) {
        if self.file_tree.root_path.as_os_str().is_empty() {
            return;
        }

        // Expand ancestors so the file becomes visible
        self.file_tree.set_expanded(path);

        // Mark it as current
        self.file_tree.set_current(path);
    }

    // Utility
    fn get_next_tab_id(&mut self) -> usize {
        let id = self.next_tab_id;
        self.next_tab_id += 1;

        id
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
    fn close_tab_returns_error_when_id_not_found() {
        let mut a = AppState::new(None).unwrap();
        let result = a.close_tab(1);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_tabs_are_empty() {
        let mut a = AppState::new(None).unwrap();
        let _ = a.close_tab(0);
        let result = a.set_active_tab(0);

        assert!(result.is_err())
    }

    #[test]
    fn set_active_tab_returns_error_when_id_not_found() {
        let mut a = AppState::new(None).unwrap();
        let result = a.set_active_tab(1);

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

        let _ = a.set_active_tab(2);
        a.pin_tab(2).unwrap();

        assert_eq!(
            a.get_tabs()
                .iter()
                .map(|t| t.get_title())
                .collect::<Vec<String>>(),
            vec!["C", "A", "B"]
        );
        assert_eq!(a.get_active_tab_id(), 2);
    }
}
