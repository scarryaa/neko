use super::bridge::ffi::*;
use crate::{
    AppState, Editor, FileTree, ShortcutsManager,
    app::Tab,
    commands::{
        execute_command,
        tab::{get_available_tab_commands, run_tab_command, tab_command_state},
    },
    config::ConfigManager,
    theme::ThemeManager,
};
use std::path::PathBuf;

pub fn new_app_state(
    root_path: &str,
    config_manager: &ConfigManager,
) -> std::io::Result<Box<AppState>> {
    let path_opt = if root_path.is_empty() {
        None
    } else {
        Some(root_path)
    };

    let state = AppState::new(config_manager, path_opt)
        .map_err(|e| std::io::Error::other(format!("Failed to initialize app state: {e}")))?;

    Ok(Box::new(state))
}

impl AppState {
    pub(crate) fn get_active_editor_wrapper(&self) -> &Editor {
        self.get_active_editor()
            .expect("Attempted to access editor but failed")
    }

    pub(crate) fn get_active_editor_mut_wrapper(&mut self) -> &mut Editor {
        self.get_active_editor_mut()
            .expect("Attempted to access mutable editor but failed")
    }

    pub fn get_close_other_tab_ids_wrapper(&self, id: usize) -> Vec<usize> {
        self.get_close_other_tab_ids(id)
            .expect("Unable to get 'close other' tab ids")
    }

    pub fn get_close_left_tab_ids_wrapper(&self, id: usize) -> Vec<usize> {
        self.get_close_left_tab_ids(id)
            .expect("Unable to get 'close left' tab ids")
    }

    pub fn get_close_right_tab_ids_wrapper(&self, id: usize) -> Vec<usize> {
        self.get_close_right_tab_ids(id)
            .expect("Unable to get 'close right' tab ids")
    }

    pub fn get_close_all_tab_ids_wrapper(&self) -> Vec<usize> {
        self.get_close_all_tab_ids()
            .expect("Unable to get 'close all' tab ids")
    }

    pub fn get_close_clean_tab_ids_wrapper(&self) -> Vec<usize> {
        self.get_close_clean_tab_ids()
            .expect("Unable to get 'close clean' tab ids")
    }

    fn make_tab_snapshot(tab: &Tab) -> TabSnapshot {
        let (path_present, path) = match tab.get_file_path() {
            Some(p) => (true, p.to_string_lossy().to_string()),
            None => (false, String::new()),
        };

        TabSnapshot {
            id: tab.get_id() as u64,
            title: tab.get_title().to_string(),
            path_present,
            path,
            modified: tab.get_modified(),
            pinned: tab.get_is_pinned(),
            scroll_offsets: ScrollOffsetFfi {
                x: tab.get_scroll_offsets().0,
                y: tab.get_scroll_offsets().1,
            },
        }
    }

    pub(crate) fn get_tab_snapshot(&self, id: usize) -> TabSnapshotMaybe {
        if let Ok(tab) = self.get_tab(id) {
            TabSnapshotMaybe {
                found: true,
                snapshot: Self::make_tab_snapshot(tab),
            }
        } else {
            TabSnapshotMaybe {
                found: false,
                snapshot: TabSnapshot::default(),
            }
        }
    }

    pub(crate) fn get_tabs_snapshot(&self) -> TabsSnapshot {
        let tabs: Vec<TabSnapshot> = self
            .get_tabs()
            .iter()
            .map(Self::make_tab_snapshot)
            .collect();

        let active_id = self.get_active_tab_id();
        let active_present = self.get_tabs().iter().any(|t| t.get_id() == active_id);

        TabsSnapshot {
            active_present,
            active_id: if active_present { active_id as u64 } else { 0 },
            tabs,
        }
    }

    pub(crate) fn open_file_wrapper(&mut self, path: &str) -> FileOpenResult {
        let result = self.open_file(path);

        match result {
            Ok(id) => {
                let tab = self.get_tab(id).unwrap();
                FileOpenResult {
                    success: true,
                    snapshot: Self::make_tab_snapshot(tab),
                }
            }
            Err(_) => FileOpenResult {
                success: false,
                snapshot: TabSnapshot::default(),
            },
        }
    }

    pub(crate) fn save_active_tab_wrapper(&mut self) -> bool {
        self.save_active_tab().is_ok()
    }

    pub(crate) fn save_active_tab_and_set_path_wrapper(&mut self, path: &str) -> bool {
        self.save_active_tab_and_set_path(path).is_ok()
    }

    pub(crate) fn new_tab_wrapper(self: &mut AppState) -> NewTabResult {
        let (id, index) = self.new_tab();
        let tab = self.get_tab(id).unwrap();

        NewTabResult {
            id: id as u64,
            index: index as u32,
            snapshot: Self::make_tab_snapshot(tab),
        }
    }

    pub(crate) fn close_tab_wrapper(&mut self, id: usize) -> CloseTabResult {
        let closed = self.close_tab(id).is_ok();

        let any_tabs_left = !self.get_tabs().is_empty();
        let active_id = if any_tabs_left {
            self.get_active_tab_id() as u64
        } else {
            0
        };

        CloseTabResult {
            closed,
            has_active: any_tabs_left,
            active_id,
        }
    }

    fn build_close_many_result(
        &self,
        success: bool,
        closed_ids: Vec<usize>,
    ) -> CloseManyTabsResult {
        let any_tabs_left = !self.get_tabs().is_empty();
        let active_id = if any_tabs_left {
            self.get_active_tab_id() as u64
        } else {
            0
        };

        CloseManyTabsResult {
            success,
            closed_ids: closed_ids.into_iter().map(|id| id as u64).collect(),
            has_active: any_tabs_left,
            active_id,
        }
    }

    pub(crate) fn close_other_tabs_wrapper(&mut self, id: usize) -> CloseManyTabsResult {
        match self.close_other_tabs(id) {
            Ok(closed_ids) => self.build_close_many_result(true, closed_ids),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub(crate) fn close_left_tabs_wrapper(&mut self, id: usize) -> CloseManyTabsResult {
        match self.close_left_tabs(id) {
            Ok(closed_ids) => self.build_close_many_result(true, closed_ids),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub(crate) fn close_right_tabs_wrapper(&mut self, id: usize) -> CloseManyTabsResult {
        match self.close_right_tabs(id) {
            Ok(closed_ids) => self.build_close_many_result(true, closed_ids),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub(crate) fn close_all_tabs_wrapper(&mut self) -> CloseManyTabsResult {
        match self.close_all_tabs() {
            Ok(closed_ids) => self.build_close_many_result(true, closed_ids),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub(crate) fn close_clean_tabs_wrapper(&mut self) -> CloseManyTabsResult {
        match self.close_clean_tabs() {
            Ok(closed_ids) => self.build_close_many_result(true, closed_ids),
            Err(_) => self.build_close_many_result(false, vec![]),
        }
    }

    pub(crate) fn move_tab_wrapper(&mut self, from: usize, to: usize) -> bool {
        self.move_tab(from, to).is_ok()
    }

    pub(crate) fn pin_tab_wrapper(&mut self, id: usize) -> PinTabResult {
        let from_index = match self.get_tabs().iter().position(|t| t.get_id() == id) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        if self.pin_tab(id).is_err() {
            return PinTabResult {
                success: false,
                from_index: 0,
                to_index: 0,
                snapshot: TabSnapshot::default(),
            };
        }

        let to_index = match self.get_tabs().iter().position(|t| t.get_id() == id) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        let tab_ref = &self.get_tabs()[to_index as usize];
        let snapshot = Self::make_tab_snapshot(tab_ref);

        PinTabResult {
            success: true,
            from_index,
            to_index,
            snapshot,
        }
    }

    pub(crate) fn unpin_tab_wrapper(&mut self, id: usize) -> PinTabResult {
        let from_index = match self.get_tabs().iter().position(|t| t.get_id() == id) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        if self.unpin_tab(id).is_err() {
            return PinTabResult {
                success: false,
                from_index: 0,
                to_index: 0,
                snapshot: TabSnapshot::default(),
            };
        }

        let to_index = match self.get_tabs().iter().position(|t| t.get_id() == id) {
            Some(idx) => idx as u32,
            None => {
                return PinTabResult {
                    success: false,
                    from_index: 0,
                    to_index: 0,
                    snapshot: TabSnapshot::default(),
                };
            }
        };

        let tab_ref = &self.get_tabs()[to_index as usize];
        let snapshot = Self::make_tab_snapshot(tab_ref);

        PinTabResult {
            success: true,
            from_index,
            to_index,
            snapshot,
        }
    }

    pub(crate) fn save_tab_with_id_wrapper(&mut self, id: usize) -> bool {
        self.save_tab_with_id(id).is_ok()
    }

    pub(crate) fn save_tab_with_id_and_set_path_wrapper(&mut self, id: usize, path: &str) -> bool {
        self.save_tab_with_id_and_set_path(id, path).is_ok()
    }

    pub(crate) fn set_tab_scroll_offsets_wrapper(
        &mut self,
        id: usize,
        new_offsets: ScrollOffsetFfi,
    ) -> bool {
        self.set_tab_scroll_offsets(id, (new_offsets.x, new_offsets.y))
            .is_ok()
    }
}

pub(crate) fn new_config_manager() -> std::io::Result<Box<ConfigManager>> {
    let config_manager = ConfigManager::new();

    Ok(Box::new(config_manager))
}

impl ConfigManager {
    pub(crate) fn get_snapshot_wrapper(&self) -> ConfigSnapshotFfi {
        self.get_snapshot().into()
    }

    pub(crate) fn apply_snapshot_wrapper(&self, snap: ConfigSnapshotFfi) {
        self.update(|c| {
            c.editor_font_size = snap.editor_font_size as usize;
            c.editor_font_family = snap.editor_font_family;

            c.file_explorer_font_size = snap.file_explorer_font_size as usize;
            c.file_explorer_font_family = snap.file_explorer_font_family;

            c.file_explorer_directory = if snap.file_explorer_directory_present {
                Some(snap.file_explorer_directory)
            } else {
                None
            };

            c.file_explorer_shown = snap.file_explorer_shown;
            c.file_explorer_width = snap.file_explorer_width as usize;
            c.file_explorer_right = snap.file_explorer_right;

            c.current_theme = snap.current_theme;
        });
    }

    pub(crate) fn get_config_path_wrapper(&self) -> String {
        if let Some(s) = ConfigManager::get_config_path().to_str() {
            s.to_string()
        } else {
            "".to_string()
        }
    }

    pub(crate) fn save_config_wrapper(&mut self) -> bool {
        self.save().is_ok()
    }
}

pub(crate) fn new_shortcuts_manager() -> std::io::Result<Box<ShortcutsManager>> {
    let shortcuts_manager = ShortcutsManager::new();

    Ok(Box::new(shortcuts_manager))
}

impl ShortcutsManager {
    pub(crate) fn get_shortcut_wrapper(&self, key: String) -> Shortcut {
        self.get_shortcut(&key)
            .map(Into::into)
            .unwrap_or_else(|| Shortcut {
                key,
                key_combo: String::new(),
            })
    }

    pub(crate) fn get_shortcuts_wrapper(&self) -> Vec<Shortcut> {
        self.get_all().into_iter().map(Into::into).collect()
    }

    pub(crate) fn add_shortcuts_wrapper(&self, shortcuts: Vec<Shortcut>) {
        let shortcuts = shortcuts
            .into_iter()
            .map(|s| crate::shortcuts::Shortcut::new(s.key, s.key_combo));

        self.add_shortcuts(shortcuts);
    }
}

pub(crate) fn new_theme_manager() -> std::io::Result<Box<ThemeManager>> {
    let theme_manager = ThemeManager::new();

    Ok(Box::new(theme_manager))
}

impl ThemeManager {
    pub(crate) fn get_color_wrapper(&mut self, key: &str) -> String {
        self.get_color(key).to_string()
    }

    pub(crate) fn get_current_theme_name_wrapper(&self) -> String {
        self.get_current_theme_name().to_string()
    }
}

impl Editor {
    pub(crate) fn get_text(&self) -> String {
        self.buffer().get_text()
    }

    pub(crate) fn get_line(&self, line_idx: usize) -> String {
        self.buffer().get_line(line_idx)
    }

    pub(crate) fn get_line_count(&self) -> usize {
        self.buffer().line_count()
    }

    pub(crate) fn get_cursor_positions(self: &Editor) -> Vec<CursorPosition> {
        self.cursors()
            .iter()
            .map(|c| CursorPosition {
                row: c.cursor.row,
                col: c.cursor.column,
            })
            .collect()
    }

    pub(crate) fn get_selection(&self) -> Selection {
        let s = self.selection().clone();
        let start = s.start.clone();
        let end = s.end.clone();
        let anchor = s.anchor.clone();
        let active = s.is_active();

        Selection {
            start: CursorPosition {
                row: start.row,
                col: start.column,
            },
            end: CursorPosition {
                row: end.row,
                col: end.column,
            },
            anchor: CursorPosition {
                row: anchor.row,
                col: anchor.column,
            },
            active,
        }
    }

    pub(crate) fn copy(self: &Editor) -> String {
        let selection = self.selection().clone();

        if selection.is_active() {
            self.buffer().get_text_range(
                selection.start.get_idx(self.buffer()),
                selection.end.get_idx(self.buffer()),
            )
        } else {
            "".to_string()
        }
    }

    pub(crate) fn paste(&mut self, text: &str) -> ChangeSetFfi {
        self.insert_text(text).into()
    }

    pub(crate) fn insert_text_wrapper(&mut self, text: &str) -> ChangeSetFfi {
        self.insert_text(text).into()
    }

    pub(crate) fn move_to_wrapper(
        &mut self,
        row: usize,
        col: usize,
        clear_selection: bool,
    ) -> ChangeSetFfi {
        self.move_to(row, col, clear_selection).into()
    }

    pub(crate) fn select_to_wrapper(&mut self, row: usize, col: usize) -> ChangeSetFfi {
        self.select_to(row, col).into()
    }

    pub(crate) fn select_word_wrapper(&mut self, row: usize, col: usize) -> ChangeSetFfi {
        self.select_word(row, col).into()
    }

    pub(crate) fn select_word_drag_wrapper(
        &mut self,
        anchor_start_row: usize,
        anchor_start_col: usize,
        anchor_end_row: usize,
        anchor_end_col: usize,
        row: usize,
        col: usize,
    ) -> ChangeSetFfi {
        self.select_word_drag(
            anchor_start_row,
            anchor_start_col,
            anchor_end_row,
            anchor_end_col,
            row,
            col,
        )
        .into()
    }

    pub(crate) fn select_line_drag_wrapper(
        &mut self,
        anchor_row: usize,
        row: usize,
    ) -> ChangeSetFfi {
        self.select_line_drag(anchor_row, row).into()
    }

    pub(crate) fn move_left_wrapper(&mut self) -> ChangeSetFfi {
        self.move_left().into()
    }

    pub(crate) fn move_right_wrapper(&mut self) -> ChangeSetFfi {
        self.move_right().into()
    }

    pub(crate) fn move_up_wrapper(&mut self) -> ChangeSetFfi {
        self.move_up().into()
    }

    pub(crate) fn move_down_wrapper(&mut self) -> ChangeSetFfi {
        self.move_down().into()
    }

    pub(crate) fn insert_newline_wrapper(&mut self) -> ChangeSetFfi {
        self.insert_text("\n").into()
    }

    pub(crate) fn insert_tab_wrapper(&mut self) -> ChangeSetFfi {
        self.insert_text("\t").into()
    }

    pub(crate) fn backspace_wrapper(&mut self) -> ChangeSetFfi {
        self.backspace().into()
    }

    pub(crate) fn delete_wrapper(&mut self) -> ChangeSetFfi {
        self.delete().into()
    }

    pub(crate) fn select_all_wrapper(&mut self) -> ChangeSetFfi {
        self.select_all().into()
    }

    pub(crate) fn select_left_wrapper(&mut self) -> ChangeSetFfi {
        self.select_left().into()
    }

    pub(crate) fn select_right_wrapper(&mut self) -> ChangeSetFfi {
        self.select_right().into()
    }

    pub(crate) fn select_up_wrapper(&mut self) -> ChangeSetFfi {
        self.select_up().into()
    }

    pub(crate) fn select_down_wrapper(&mut self) -> ChangeSetFfi {
        self.select_down().into()
    }

    pub(crate) fn clear_selection_wrapper(&mut self) -> ChangeSetFfi {
        self.clear_selection().into()
    }

    pub(crate) fn clear_cursors_wrapper(&mut self) -> ChangeSetFfi {
        self.clear_cursors().into()
    }

    pub(crate) fn undo_wrapper(&mut self) -> ChangeSetFfi {
        self.undo().into()
    }

    pub(crate) fn redo_wrapper(&mut self) -> ChangeSetFfi {
        self.redo().into()
    }

    pub(crate) fn get_max_width(&self) -> f64 {
        self.widths().max_width()
    }

    pub(crate) fn add_cursor_wrapper(self: &mut Editor, direction: AddCursorDirectionFfi) {
        self.add_cursor(direction.into());
    }

    pub(crate) fn get_last_added_cursor(&self) -> CursorPosition {
        self.last_added_cursor().into()
    }
}

impl FileTree {
    pub(crate) fn get_next_node(&self, current_path: &str) -> FileNodeSnapshot {
        let node = self.next(current_path).unwrap();
        self.make_file_node_snapshot(node)
    }

    pub(crate) fn get_prev_node(&self, current_path: &str) -> FileNodeSnapshot {
        let node = self.prev(current_path).unwrap();
        self.make_file_node_snapshot(node)
    }

    pub(crate) fn get_path_of_parent(&self, path: &str) -> String {
        let path_buf = PathBuf::from(path);
        path_buf
            .parent()
            .unwrap()
            .to_string_lossy()
            .as_ref()
            .to_string()
    }

    pub(crate) fn get_children_wrapper(&mut self, path: &str) -> Vec<FileNodeSnapshot> {
        let children_owned = self.get_children(path).to_vec();

        children_owned
            .iter()
            .map(|node| self.make_file_node_snapshot(node))
            .collect()
    }

    fn make_file_node_snapshot(&self, node: &crate::FileNode) -> FileNodeSnapshot {
        let p = PathBuf::from(node.path_str());

        FileNodeSnapshot {
            path: node.path.clone(),
            name: node.name.clone(),
            is_dir: node.is_dir,
            is_hidden: node.is_hidden,
            is_expanded: node.is_dir && self.get_expanded_owned().contains_key(&p),
            is_selected: self.get_selected_owned().contains(&p),
            is_current: self.get_current().as_ref() == Some(&p),
            size: node.size,
            modified: node.modified,
            depth: node.depth,
        }
    }

    pub(crate) fn get_tree_snapshot(&self) -> FileTreeSnapshot {
        let nodes: Vec<FileNodeSnapshot> = self
            .get_visible_nodes_owned()
            .iter()
            .map(|node| self.make_file_node_snapshot(node))
            .collect();

        let root_present = !self.root_path.as_os_str().is_empty();

        FileTreeSnapshot {
            root_present,
            root: self.root_path.to_string_lossy().to_string(),
            nodes,
        }
    }
}

pub(crate) fn execute_command_wrapper(
    cmd: CommandFfi,
    config: &mut ConfigManager,
    theme: &mut ThemeManager,
) -> CommandResultFfi {
    execute_command(cmd.into(), config, theme).into()
}

pub(crate) fn new_command(kind: CommandKindFfi, argument: String) -> CommandFfi {
    CommandFfi { kind, argument }
}

// Tab commands
pub(crate) fn get_tab_command_state(app_state: &AppState, id: u64) -> TabCommandStateFfi {
    match tab_command_state(app_state, id as usize) {
        Ok(state) => state.into(),
        Err(_) => TabCommandStateFfi {
            can_close: false,
            can_close_others: false,
            can_close_left: false,
            can_close_right: false,
            can_close_all: false,
            can_close_clean: false,
            can_copy_path: false,
            can_reveal: false,
            is_pinned: false,
        },
    }
}

pub(crate) fn run_tab_command_wrapper(
    app_state: &mut AppState,
    id: &str,
    ctx: TabContextFfi,
) -> bool {
    run_tab_command(app_state, id, &ctx.into()).is_ok()
}

pub fn get_available_tab_commands_wrapper() -> Vec<TabCommandFfi> {
    let Ok(commands) = get_available_tab_commands() else {
        return Vec::new();
    };

    commands
        .into_iter()
        .map(|cmd| TabCommandFfi {
            id: cmd.as_str().to_string(),
        })
        .collect()
}
