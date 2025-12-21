use super::bridge::ffi::*;
use crate::{
    AppState, Editor, FileTree, ShortcutsManager, commands::execute_command, config::ConfigManager,
    theme::ThemeManager,
};
use std::path::PathBuf;

pub fn new_app_state(root_path: &str) -> std::io::Result<Box<AppState>> {
    let path_opt = if root_path.is_empty() {
        None
    } else {
        Some(root_path)
    };

    let state = AppState::new(path_opt)
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

    pub(crate) fn get_active_tab_path_wrapper(&self) -> String {
        self.get_active_tab_path()
            .expect("Tried to access active tab path but failed")
            .to_string()
    }

    pub(crate) fn get_tab_path_wrapper(&self, index: usize) -> String {
        self.get_tab_path(index)
            .expect("Tried to access tab path but failed")
            .to_string()
    }

    pub(crate) fn get_tab_index_by_path_wrapper(&self, path: &str) -> i64 {
        self.get_tab_index_by_path(path)
            .map(|i| i as i64)
            .unwrap_or(-1)
    }

    pub(crate) fn open_file_wrapper(&mut self, path: &str) -> bool {
        self.open_file(path).is_ok()
    }

    pub(crate) fn save_active_tab_wrapper(&mut self) -> bool {
        self.save_active_tab().is_ok()
    }

    pub(crate) fn save_active_tab_and_set_path_wrapper(&mut self, path: &str) -> bool {
        self.save_active_tab_and_set_path(path).is_ok()
    }

    pub(crate) fn close_tab_wrapper(&mut self, index: usize) -> bool {
        self.close_tab(index).is_ok()
    }

    pub(crate) fn close_other_tabs_wrapper(self: &mut AppState, index: usize) -> bool {
        self.close_other_tabs(index).is_ok()
    }

    pub(crate) fn close_left_tabs_wrapper(self: &mut AppState, index: usize) -> bool {
        self.close_left_tabs(index).is_ok()
    }

    pub(crate) fn close_right_tabs_wrapper(self: &mut AppState, index: usize) -> bool {
        self.close_right_tabs(index).is_ok()
    }

    pub(crate) fn move_tab_wrapper(self: &mut AppState, from: usize, to: usize) -> bool {
        self.move_tab(from, to).is_ok()
    }

    pub(crate) fn pin_tab_wrapper(self: &mut AppState, index: usize) -> bool {
        self.pin_tab(index).is_ok()
    }

    pub(crate) fn unpin_tab_wrapper(self: &mut AppState, index: usize) -> bool {
        self.unpin_tab(index).is_ok()
    }
}

pub(crate) fn new_config_manager() -> std::io::Result<Box<ConfigManager>> {
    let config_manager = ConfigManager::new();

    Ok(Box::new(config_manager))
}

impl ConfigManager {
    pub(crate) fn get_font_size(&self, font_type: FontType) -> usize {
        match font_type {
            FontType::Editor => self.get_snapshot().editor_font_size,
            FontType::FileExplorer => self.get_snapshot().file_explorer_font_size,
            FontType::Interface => 15,
            FontType::Terminal => 15,
            _ => 15,
        }
    }

    pub(crate) fn get_config_path_wrapper(&self) -> String {
        if let Some(s) = ConfigManager::get_config_path().to_str() {
            s.to_string()
        } else {
            "".to_string()
        }
    }

    pub(crate) fn get_font_family(&self, font_type: FontType) -> String {
        match font_type {
            FontType::Editor => self.get_snapshot().editor_font_family,
            FontType::FileExplorer => self.get_snapshot().file_explorer_font_family,
            FontType::Interface => "IBM Plex Sans".to_string(),
            FontType::Terminal => "IBM Plex Mono".to_string(),
            _ => "IBM Plex Sans".to_string(),
        }
    }

    pub(crate) fn get_file_explorer_directory(&self) -> String {
        self.get_snapshot()
            .file_explorer_directory
            .unwrap_or("".to_string())
            .to_string()
    }

    pub(crate) fn save_config_wrapper(&mut self) -> bool {
        self.save().is_ok()
    }

    pub(crate) fn set_font_size(&mut self, font_size: usize, font_type: FontType) {
        self.update(|c| match font_type {
            FontType::Editor => c.editor_font_size = font_size,
            FontType::FileExplorer => c.file_explorer_font_size = font_size,
            FontType::Interface => todo!(),
            FontType::Terminal => todo!(),
            _ => todo!(),
        });
    }

    pub(crate) fn set_font_family(&mut self, font_family: &str, font_type: FontType) {
        self.update(|c| match font_type {
            FontType::Editor => c.editor_font_family = font_family.to_string(),
            FontType::FileExplorer => c.file_explorer_font_family = font_family.to_string(),
            FontType::Interface => todo!(),
            FontType::Terminal => todo!(),
            _ => todo!(),
        });
    }

    pub(crate) fn set_file_explorer_directory(&mut self, new_directory: &str) {
        self.update(|c| c.file_explorer_directory = Some(new_directory.to_string()));
    }

    pub(crate) fn set_file_explorer_shown(self: &mut ConfigManager, shown: bool) {
        self.update(|c| c.file_explorer_shown = shown);
    }

    pub(crate) fn get_file_explorer_shown(self: &ConfigManager) -> bool {
        self.get_snapshot().file_explorer_shown
    }

    pub(crate) fn set_file_explorer_width(&mut self, width: usize) {
        self.update(|c| c.file_explorer_width = width);
    }

    pub(crate) fn get_file_explorer_width(&self) -> usize {
        self.get_snapshot().file_explorer_width
    }

    pub(crate) fn get_file_explorer_right(self: &ConfigManager) -> bool {
        self.get_snapshot().file_explorer_right
    }

    pub(crate) fn set_file_explorer_right(self: &ConfigManager, is_right: bool) {
        self.update(|c| c.file_explorer_right = is_right);
    }

    pub(crate) fn set_theme(self: &ConfigManager, theme_name: String) {
        self.update(|c| c.current_theme = theme_name);
    }

    pub(crate) fn get_theme(&self) -> String {
        self.get_snapshot().current_theme.clone()
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
    pub(crate) fn get_children_wrapper(&mut self, path: &str) -> Vec<FileNode> {
        self.get_children(path)
            .iter()
            .map(|n| FileNode {
                path: n.path_str(),
                name: n.name_str(),
                is_dir: n.is_dir,
                is_hidden: n.is_hidden,
                size: n.size,
                modified: n.modified,
                depth: n.depth,
            })
            .collect()
    }

    pub(crate) fn get_node_wrapper(&self, path: &str) -> FileNode {
        let node = self.get_node(path).unwrap();

        FileNode {
            path: node.path_str(),
            name: node.name_str(),
            is_dir: node.is_dir,
            is_hidden: node.is_hidden,
            size: node.size,
            modified: node.modified,
            depth: node.depth,
        }
    }

    pub(crate) fn get_next_node(&self, current_path: &str) -> FileNode {
        let node = self.next(current_path).unwrap();

        FileNode {
            path: node.path_str(),
            name: node.name_str(),
            is_dir: node.is_dir,
            is_hidden: node.is_hidden,
            size: node.size,
            modified: node.modified,
            depth: node.depth,
        }
    }

    pub(crate) fn get_prev_node(&self, current_path: &str) -> FileNode {
        let node = self.prev(current_path).unwrap();

        FileNode {
            path: node.path_str(),
            name: node.name_str(),
            is_dir: node.is_dir,
            is_hidden: node.is_hidden,
            size: node.size,
            modified: node.modified,
            depth: node.depth,
        }
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

    pub(crate) fn get_visible_nodes_wrapper(tree: &FileTree) -> Vec<FileNode> {
        tree.get_visible_nodes_owned()
            .iter()
            .map(|n| FileNode {
                path: n.path_str(),
                name: n.name_str(),
                is_dir: n.is_dir,
                is_hidden: n.is_hidden,
                size: n.size,
                modified: n.modified,
                depth: n.depth,
            })
            .collect()
    }

    pub(crate) fn get_path_of_current(&self) -> String {
        self.get_current()
            .unwrap_or("".into())
            .to_string_lossy()
            .as_ref()
            .to_string()
    }
}

impl FileNode {
    pub(crate) fn get_path(&self) -> String {
        self.path.clone()
    }

    pub(crate) fn get_name(&self) -> String {
        self.name.clone()
    }

    pub(crate) fn get_is_dir(&self) -> bool {
        self.is_dir
    }

    pub(crate) fn get_is_hidden(&self) -> bool {
        self.is_hidden
    }

    pub(crate) fn get_size(&self) -> u64 {
        self.size
    }

    pub(crate) fn get_modified(&self) -> u64 {
        self.modified
    }

    pub(crate) fn get_depth(&self) -> usize {
        self.depth
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
