use crate::{
    Config, ConfigManager, Editor, FileTree, JumpHistory, MoveActiveTabResult, Tab, TabManager,
};
use std::io::Error;

/// Application state.
///
/// Acts primarily as a facade over [`TabManager`] and [`FileTree`],
/// delegating most tab and file-tree operations to those components.
#[derive(Debug)]
pub struct AppState {
    file_tree: FileTree,
    config_manager: *const ConfigManager,
    tab_manager: TabManager,
    pub jump_history: JumpHistory,
}

impl AppState {
    pub fn new(
        config_manager: &ConfigManager,
        root_path: Option<&str>,
    ) -> Result<Self, std::io::Error> {
        Ok(Self {
            file_tree: FileTree::new(root_path)?,
            config_manager,
            tab_manager: TabManager::new()?,
            jump_history: JumpHistory::default(),
        })
    }

    // Getters
    pub fn get_tabs(&self) -> &Vec<Tab> {
        self.tab_manager.get_tabs()
    }

    pub fn get_tab(&self, id: usize) -> Result<&Tab, Error> {
        self.tab_manager.get_tab(id)
    }

    pub fn get_active_tab_id(&self) -> usize {
        self.tab_manager.get_active_tab_id()
    }

    pub fn get_active_editor(&self) -> Option<&Editor> {
        self.tab_manager.get_active_editor()
    }

    pub fn get_active_editor_mut(&mut self) -> Option<&mut Editor> {
        self.tab_manager.get_active_editor_mut()
    }

    pub fn get_close_other_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        self.tab_manager.get_close_other_tab_ids(id)
    }

    pub fn get_close_all_tab_ids(&self) -> Result<Vec<usize>, Error> {
        self.tab_manager.get_close_all_tab_ids()
    }

    pub fn get_close_clean_tab_ids(&self) -> Result<Vec<usize>, Error> {
        self.tab_manager.get_close_clean_tab_ids()
    }

    pub fn get_close_left_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        self.tab_manager.get_close_left_tab_ids(id)
    }

    pub fn get_close_right_tab_ids(&self, id: usize) -> Result<Vec<usize>, Error> {
        self.tab_manager.get_close_right_tab_ids(id)
    }

    pub fn get_config_snapshot(&self) -> Config {
        unsafe { &*self.config_manager }.get_snapshot()
    }

    // Setters
    pub fn new_tab(&mut self, add_to_history: bool) -> (usize, usize) {
        self.tab_manager.new_tab(add_to_history)
    }

    pub fn close_tab(&mut self, id: usize) -> Result<(), Error> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;
        self.tab_manager.close_tab(id, history_enabled)
    }

    pub fn close_other_tabs(&mut self, id: usize) -> Result<Vec<usize>, Error> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;
        self.tab_manager.close_other_tabs(id, history_enabled)
    }

    pub fn close_left_tabs(&mut self, id: usize) -> Result<Vec<usize>, Error> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;
        self.tab_manager.close_left_tabs(id, history_enabled)
    }

    pub fn close_right_tabs(&mut self, id: usize) -> Result<Vec<usize>, Error> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;
        self.tab_manager.close_right_tabs(id, history_enabled)
    }

    pub fn close_all_tabs(&mut self) -> Result<Vec<usize>, Error> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;
        self.tab_manager.close_all_tabs(history_enabled)
    }

    pub fn close_clean_tabs(&mut self) -> Result<Vec<usize>, Error> {
        let history_enabled = self
            .config_manager()
            .get_snapshot()
            .editor
            .switch_to_last_visited_tab_on_close;
        self.tab_manager.close_clean_tabs(history_enabled)
    }

    pub fn move_active_tab_by(&mut self, delta: i64, use_history: bool) -> MoveActiveTabResult {
        let auto_reopen_closed_tabs_in_history = self
            .config_manager()
            .get_snapshot()
            .editor
            .auto_reopen_closed_tabs_in_history;

        let result = self.tab_manager.move_active_tab_by(
            delta,
            use_history,
            auto_reopen_closed_tabs_in_history,
        );

        // Restore cursors, selections for reopened tab
        if let Some(found_id) = result.found_id {
            if let Ok(tab) = self.tab_manager.get_tab_mut(found_id) {
                let editor = tab.get_editor_mut();

                if let Some(ref cursors) = result.cursors {
                    editor.cursor_manager_mut().set_cursors(cursors.to_vec());
                }

                if let Some(ref selections) = result.selections {
                    editor.selection_manager_mut().set_selection(selections);
                }
            }
        }

        result
    }

    pub fn pin_tab(&mut self, id: usize) -> Result<(), Error> {
        self.tab_manager.pin_tab(id)
    }

    pub fn unpin_tab(&mut self, id: usize) -> Result<(), Error> {
        self.tab_manager.unpin_tab(id)
    }

    pub fn set_tab_scroll_offsets(
        &mut self,
        id: usize,
        new_offsets: (i32, i32),
    ) -> Result<(), Error> {
        self.tab_manager.set_tab_scroll_offsets(id, new_offsets)
    }

    pub fn set_active_tab(&mut self, id: usize) -> Result<(), Error> {
        self.tab_manager.set_active_tab(id)
    }

    pub fn move_tab(&mut self, from: usize, to: usize) -> Result<(), Error> {
        self.tab_manager.move_tab(from, to)
    }

    pub fn open_file(&mut self, path: &str) -> Result<usize, std::io::Error> {
        self.tab_manager.open_file(path)
    }

    pub fn save_active_tab(&mut self) -> Result<(), std::io::Error> {
        self.tab_manager.save_active_tab()
    }

    pub fn save_active_tab_and_set_path(&mut self, path: &str) -> Result<(), std::io::Error> {
        self.tab_manager.save_active_tab_and_set_path(path)
    }

    pub fn save_tab_with_id(&mut self, id: usize) -> Result<(), Error> {
        let result = self.tab_manager.save_tab_with_id(id);

        if result.is_ok() {
            if let Ok(tab) = self.tab_manager.get_tab(id) {
                if let Some(path) = tab.get_file_path() {
                    if path == ConfigManager::get_config_path() {
                        // Refresh config if tab with config path was saved in the editor
                        // TODO(scarlet): Run a file watcher or call reload before writing via
                        // update fn to accommodate external changes eventually
                        if let Err(e) = self.config_manager().reload_from_disk() {
                            eprintln!("Failed to reload config: {e}");
                        }
                    }
                }
            }
        }

        result
    }

    pub fn save_tab_with_id_and_set_path(&mut self, id: usize, path: &str) -> Result<(), Error> {
        self.tab_manager.save_tab_with_id_and_set_path(id, path)
    }

    pub fn get_file_tree(&self) -> &FileTree {
        &self.file_tree
    }

    pub fn get_file_tree_mut(&mut self) -> &mut FileTree {
        &mut self.file_tree
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
    pub fn with_editor<F>(&mut self, mut f: F)
    where
        F: FnMut(&mut Editor),
    {
        if let Some(editor) = self.get_active_editor_mut() {
            f(editor);
        }
    }

    fn config_manager(&self) -> &ConfigManager {
        // SAFETY: C++ must ensure ConfigManager outlives AppState
        assert!(
            !self.config_manager.is_null(),
            "config_manager pointer is null"
        );
        unsafe { &*self.config_manager }
    }
}
