use std::path::PathBuf;

use bridge::{
    AddCursorDirectionFfi, AddCursorDirectionKind, ChangeSetFfi, CursorPosition, FileNode,
    FontType, Selection,
};

use crate::app::AppState;
use crate::config::ConfigManager;
use crate::file_system::FileTree;
use crate::text::cursor_manager::AddCursorDirection;
use crate::text::{ChangeSet, Editor};
use crate::theme::ThemeManager;

impl From<ChangeSet> for bridge::ChangeSetFfi {
    fn from(cs: ChangeSet) -> Self {
        Self {
            mask: cs.change.bits(),
            line_count_before: cs.line_count_before as u32,
            line_count_after: cs.line_count_after as u32,
            dirty_first_row: cs.dirty_first_row.map(|v| v as i32).unwrap_or(-1),
            dirty_last_row: cs.dirty_last_row.map(|v| v as i32).unwrap_or(-1),
        }
    }
}

impl From<AddCursorDirectionFfi> for AddCursorDirection {
    fn from(dir: AddCursorDirectionFfi) -> Self {
        match dir.kind {
            AddCursorDirectionKind::Above => AddCursorDirection::Above,
            AddCursorDirectionKind::Below => AddCursorDirection::Below,
            AddCursorDirectionKind::At => AddCursorDirection::At {
                row: dir.row,
                col: dir.col,
            },
            _ => AddCursorDirection::Above,
        }
    }
}

#[cxx::bridge(namespace = "neko")]
mod bridge {
    enum FontType {
        Editor,
        FileExplorer,
        Interface,
        Terminal,
    }

    struct CursorPosition {
        row: usize,
        col: usize,
    }

    struct Selection {
        start: CursorPosition,
        end: CursorPosition,
        anchor: CursorPosition,
        active: bool,
    }

    struct FileNode {
        path: String,
        name: String,
        is_dir: bool,
        is_hidden: bool,
        size: u64,
        modified: u64,
        depth: usize,
    }

    struct ChangeSetFfi {
        mask: u32,
        line_count_before: u32,
        line_count_after: u32,
        dirty_first_row: i32,
        dirty_last_row: i32,
    }

    enum AddCursorDirectionKind {
        Above,
        Below,
        At,
    }

    struct AddCursorDirectionFfi {
        kind: AddCursorDirectionKind,
        row: usize,
        col: usize,
    }

    extern "Rust" {
        type AppState;
        type ConfigManager;
        type ThemeManager;
        type Editor;
        type FileTree;

        // AppState
        fn new_app_state(root_path: &str) -> Result<Box<AppState>>;
        fn is_file_open(self: &AppState, path: &str) -> bool;
        #[cxx_name = "get_tab_index_by_path"]
        fn get_tab_index_by_path_wrapper(self: &AppState, path: &str) -> i64;
        #[cxx_name = "open_file"]
        fn open_file_wrapper(self: &mut AppState, path: &str) -> bool;
        fn save_active_file(self: &mut AppState) -> bool;
        #[cxx_name = "save_and_set_path"]
        fn save_and_set_path_wrapper(self: &mut AppState, path: &str) -> bool;
        fn get_editor_mut(self: &mut AppState) -> &mut Editor;
        fn get_tab_titles(self: &AppState) -> Vec<String>;
        fn get_tab_modified_states(self: &AppState) -> Vec<bool>;
        fn get_tab_modified(self: &AppState, index: usize) -> bool;
        fn get_tab_count(self: &AppState) -> usize;
        fn get_active_tab_index(self: &AppState) -> usize;
        #[cxx_name = "close_tab"]
        fn close_tab_wrapper(self: &mut AppState, index: usize) -> bool;
        fn set_active_tab_index(self: &mut AppState, index: usize);
        fn new_tab(self: &mut AppState) -> usize;
        fn get_file_tree_mut(self: &mut AppState) -> &mut FileTree;

        // ConfigManager
        fn new_config_manager() -> Result<Box<ConfigManager>>;
        fn get_font_size(self: &ConfigManager, font_type: FontType) -> usize;
        fn get_font_family(self: &ConfigManager, font_type: FontType) -> String;
        #[cxx_name = "save_config"]
        fn save_config_wrapper(self: &mut ConfigManager) -> bool;
        fn set_font_size(self: &mut ConfigManager, font_size: usize, font_type: FontType);
        fn set_font_family(self: &mut ConfigManager, font_family: &str, font_type: FontType);
        fn set_file_explorer_directory(self: &mut ConfigManager, new_directory: &str);
        fn get_file_explorer_directory(self: &ConfigManager) -> String;
        fn set_file_explorer_shown(self: &mut ConfigManager, shown: bool);
        fn get_file_explorer_shown(self: &ConfigManager) -> bool;
        fn set_file_explorer_width(self: &mut ConfigManager, width: usize);
        fn get_file_explorer_width(self: &ConfigManager) -> usize;

        // ThemeManager
        fn new_theme_manager() -> Result<Box<ThemeManager>>;
        #[cxx_name = "get_color"]
        fn get_color_wrapper(self: &mut ThemeManager, key: &str) -> String;

        // Editor
        #[cxx_name = "move_to"]
        fn move_to_wrapper(
            self: &mut Editor,
            row: usize,
            col: usize,
            clear_selection: bool,
        ) -> ChangeSetFfi;
        #[cxx_name = "select_to"]
        fn select_to_wrapper(self: &mut Editor, row: usize, col: usize) -> ChangeSetFfi;
        #[cxx_name = "move_left"]
        fn move_left_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "move_right"]
        fn move_right_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "move_up"]
        fn move_up_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "move_down"]
        fn move_down_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "insert_text"]
        fn insert_text_wrapper(self: &mut Editor, text: &str) -> ChangeSetFfi;
        #[cxx_name = "insert_newline"]
        fn insert_newline_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "insert_tab"]
        fn insert_tab_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "backspace"]
        fn backspace_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "delete_forwards"]
        fn delete_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_all"]
        fn select_all_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_left"]
        fn select_left_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_right"]
        fn select_right_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_up"]
        fn select_up_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_down"]
        fn select_down_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "clear_selection"]
        fn clear_selection_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "undo"]
        fn undo_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "redo"]
        fn redo_wrapper(self: &mut Editor) -> ChangeSetFfi;
        fn get_max_width(self: &Editor) -> f64;
        #[cxx_name = "set_line_width"]
        fn update_line_width(self: &mut Editor, line_idx: usize, line_width: f64);
        fn needs_width_measurement(self: &Editor, line_idx: usize) -> bool;
        fn get_text(self: &Editor) -> String;
        fn get_line(self: &Editor, line_idx: usize) -> String;
        fn get_line_count(self: &Editor) -> usize;
        fn get_cursor_positions(self: &Editor) -> Vec<CursorPosition>;
        fn get_selection(self: &mut Editor) -> Selection;
        fn copy(self: &Editor) -> String;
        fn paste(self: &mut Editor, text: &str) -> ChangeSetFfi;
        #[cxx_name = "add_cursor"]
        fn add_cursor_wrapper(self: &mut Editor, direction: AddCursorDirectionFfi);
        fn remove_cursor(self: &mut Editor, row: usize, col: usize);
        fn cursor_exists_at(self: &Editor, row: usize, col: usize) -> bool;
        #[cxx_name = "clear_cursors"]
        fn clear_cursors_wrapper(self: &mut Editor) -> ChangeSetFfi;
        fn has_active_selection(self: &Editor) -> bool;
        fn cursor_exists_at_row(self: &Editor, row: usize) -> bool;

        // FileTree
        #[cxx_name = "set_root_dir"]
        fn set_root_path(self: &mut FileTree, path: &str);
        #[cxx_name = "get_children"]
        fn get_children_wrapper(self: &mut FileTree, path: &str) -> Vec<FileNode>;
        #[cxx_name = "get_node"]
        fn get_node_wrapper(self: &FileTree, path: &str) -> FileNode;
        fn get_next_node(self: &FileTree, current_path: &str) -> FileNode;
        fn get_prev_node(self: &FileTree, current_path: &str) -> FileNode;
        fn get_path_of_parent(self: &FileTree, path: &str) -> String;
        fn toggle_expanded(self: &mut FileTree, path: &str);
        fn set_expanded(self: &mut FileTree, path: &str);
        fn set_collapsed(self: &mut FileTree, path: &str);
        #[cxx_name = "get_visible_nodes"]
        fn get_visible_nodes_wrapper(self: &mut FileTree) -> Vec<FileNode>;
        fn toggle_select(self: &mut FileTree, path: &str);
        fn set_current(self: &mut FileTree, path: &str);
        fn clear_current(self: &mut FileTree);
        #[cxx_name = "get_current_index"]
        fn get_index(self: &FileTree) -> usize;
        fn get_path_of_current(self: &FileTree) -> String;
        fn is_expanded(self: &FileTree, path: &str) -> bool;
        fn is_selected(self: &FileTree, path: &str) -> bool;
        fn is_current(self: &FileTree, path: &str) -> bool;
        fn refresh_dir(self: &mut FileTree, path: &str);

        // FileNode
        fn get_path(self: &FileNode) -> String;
        fn get_name(self: &FileNode) -> String;
        fn get_is_dir(self: &FileNode) -> bool;
        fn get_is_hidden(self: &FileNode) -> bool;
        fn get_size(self: &FileNode) -> u64;
        fn get_modified(self: &FileNode) -> u64;
        fn get_depth(self: &FileNode) -> usize;
    }
}

fn new_app_state(root_path: &str) -> std::io::Result<Box<AppState>> {
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
    fn get_editor_mut(&mut self) -> &mut Editor {
        self.get_active_editor_mut()
            .expect("Attempted to access editor but failed")
    }

    fn is_file_open(&self, path: &str) -> bool {
        self.tab_with_path_exists(path)
    }

    fn get_tab_index_by_path_wrapper(&self, path: &str) -> i64 {
        self.get_tab_index_by_path(path)
            .map(|i| i as i64)
            .unwrap_or(-1)
    }

    fn open_file_wrapper(&mut self, path: &str) -> bool {
        self.open_file(path).is_ok()
    }

    fn save_active_file(&mut self) -> bool {
        self.save_file().is_ok()
    }

    fn save_and_set_path_wrapper(&mut self, path: &str) -> bool {
        self.save_and_set_path(path).is_ok()
    }

    fn close_tab_wrapper(&mut self, index: usize) -> bool {
        self.close_tab(index).is_ok()
    }
}

fn new_config_manager() -> std::io::Result<Box<ConfigManager>> {
    let config_manager = ConfigManager::new();

    Ok(Box::new(config_manager))
}

impl ConfigManager {
    fn get_font_size(&self, font_type: FontType) -> usize {
        match font_type {
            FontType::Editor => self.get_snapshot().editor_font_size,
            FontType::FileExplorer => self.get_snapshot().file_explorer_font_size,
            FontType::Interface => 15,
            FontType::Terminal => 15,
            _ => 15,
        }
    }

    fn get_font_family(&self, font_type: FontType) -> String {
        match font_type {
            FontType::Editor => self.get_snapshot().editor_font_family,
            FontType::FileExplorer => self.get_snapshot().file_explorer_font_family,
            FontType::Interface => "IBM Plex Sans".to_string(),
            FontType::Terminal => "IBM Plex Mono".to_string(),
            _ => "IBM Plex Sans".to_string(),
        }
    }

    fn get_file_explorer_directory(&self) -> String {
        self.get_snapshot()
            .file_explorer_directory
            .unwrap_or("".to_string())
            .to_string()
    }

    fn save_config_wrapper(&mut self) -> bool {
        self.save().is_ok()
    }

    fn set_font_size(&mut self, font_size: usize, font_type: FontType) {
        self.update(|c| match font_type {
            FontType::Editor => c.editor_font_size = font_size,
            FontType::FileExplorer => c.file_explorer_font_size = font_size,
            FontType::Interface => todo!(),
            FontType::Terminal => todo!(),
            _ => todo!(),
        });
    }

    fn set_font_family(&mut self, font_family: &str, font_type: FontType) {
        self.update(|c| match font_type {
            FontType::Editor => c.editor_font_family = font_family.to_string(),
            FontType::FileExplorer => c.file_explorer_font_family = font_family.to_string(),
            FontType::Interface => todo!(),
            FontType::Terminal => todo!(),
            _ => todo!(),
        });
    }

    fn set_file_explorer_directory(&mut self, new_directory: &str) {
        self.update(|c| c.file_explorer_directory = Some(new_directory.to_string()));
    }

    fn set_file_explorer_shown(self: &mut ConfigManager, shown: bool) {
        self.update(|c| c.file_explorer_shown = shown);
    }

    fn get_file_explorer_shown(self: &ConfigManager) -> bool {
        self.get_snapshot().file_explorer_shown
    }

    fn set_file_explorer_width(&mut self, width: usize) {
        self.update(|c| c.file_explorer_width = width);
    }

    fn get_file_explorer_width(&self) -> usize {
        self.get_snapshot().file_explorer_width
    }
}

fn new_theme_manager() -> std::io::Result<Box<ThemeManager>> {
    let theme_manager = ThemeManager::new();

    Ok(Box::new(theme_manager))
}

impl ThemeManager {
    fn get_color_wrapper(&mut self, key: &str) -> String {
        self.get_color(key).to_string()
    }
}

impl Editor {
    fn get_text(&self) -> String {
        self.buffer().get_text()
    }

    fn get_line(&self, line_idx: usize) -> String {
        self.buffer().get_line(line_idx)
    }

    fn get_line_count(&self) -> usize {
        self.buffer().line_count()
    }

    fn get_cursor_positions(self: &Editor) -> Vec<CursorPosition> {
        self.cursors()
            .iter()
            .map(|c| CursorPosition {
                row: c.cursor.row,
                col: c.cursor.column,
            })
            .collect()
    }

    fn get_selection(&self) -> Selection {
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

    fn copy(self: &Editor) -> String {
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

    fn paste(&mut self, text: &str) -> ChangeSetFfi {
        self.insert_text(text).into()
    }

    fn insert_text_wrapper(&mut self, text: &str) -> ChangeSetFfi {
        self.insert_text(text).into()
    }

    fn move_to_wrapper(&mut self, row: usize, col: usize, clear_selection: bool) -> ChangeSetFfi {
        self.move_to(row, col, clear_selection).into()
    }

    fn select_to_wrapper(&mut self, row: usize, col: usize) -> ChangeSetFfi {
        self.select_to(row, col).into()
    }

    fn move_left_wrapper(&mut self) -> ChangeSetFfi {
        self.move_left().into()
    }

    fn move_right_wrapper(&mut self) -> ChangeSetFfi {
        self.move_right().into()
    }

    fn move_up_wrapper(&mut self) -> ChangeSetFfi {
        self.move_up().into()
    }

    fn move_down_wrapper(&mut self) -> ChangeSetFfi {
        self.move_down().into()
    }

    fn insert_newline_wrapper(&mut self) -> ChangeSetFfi {
        self.insert_text("\n").into()
    }

    fn insert_tab_wrapper(&mut self) -> ChangeSetFfi {
        self.insert_text("\t").into()
    }

    fn backspace_wrapper(&mut self) -> ChangeSetFfi {
        self.backspace().into()
    }

    fn delete_wrapper(&mut self) -> ChangeSetFfi {
        self.delete().into()
    }

    fn select_all_wrapper(&mut self) -> ChangeSetFfi {
        self.select_all().into()
    }

    fn select_left_wrapper(&mut self) -> ChangeSetFfi {
        self.select_left().into()
    }

    fn select_right_wrapper(&mut self) -> ChangeSetFfi {
        self.select_right().into()
    }

    fn select_up_wrapper(&mut self) -> ChangeSetFfi {
        self.select_up().into()
    }

    fn select_down_wrapper(&mut self) -> ChangeSetFfi {
        self.select_down().into()
    }

    fn clear_selection_wrapper(&mut self) -> ChangeSetFfi {
        self.clear_selection().into()
    }

    fn clear_cursors_wrapper(&mut self) -> ChangeSetFfi {
        self.clear_cursors().into()
    }

    fn undo_wrapper(&mut self) -> ChangeSetFfi {
        self.undo().into()
    }

    fn redo_wrapper(&mut self) -> ChangeSetFfi {
        self.redo().into()
    }

    fn get_max_width(&self) -> f64 {
        self.widths().max_width()
    }

    fn add_cursor_wrapper(self: &mut Editor, direction: AddCursorDirectionFfi) {
        self.add_cursor(direction.into());
    }
}

impl FileTree {
    fn get_children_wrapper(&mut self, path: &str) -> Vec<FileNode> {
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

    fn get_node_wrapper(&self, path: &str) -> FileNode {
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

    fn get_next_node(&self, current_path: &str) -> FileNode {
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

    fn get_prev_node(&self, current_path: &str) -> FileNode {
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

    fn get_path_of_parent(&self, path: &str) -> String {
        let path_buf = PathBuf::from(path);
        path_buf
            .parent()
            .unwrap()
            .to_string_lossy()
            .as_ref()
            .to_string()
    }

    fn get_visible_nodes_wrapper(tree: &FileTree) -> Vec<FileNode> {
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

    fn get_path_of_current(&self) -> String {
        self.get_current()
            .unwrap_or("".into())
            .to_string_lossy()
            .as_ref()
            .to_string()
    }
}

impl FileNode {
    fn get_path(&self) -> String {
        self.path.clone()
    }

    fn get_name(&self) -> String {
        self.name.clone()
    }

    fn get_is_dir(&self) -> bool {
        self.is_dir
    }

    fn get_is_hidden(&self) -> bool {
        self.is_hidden
    }

    fn get_size(&self) -> u64 {
        self.size
    }

    fn get_modified(&self) -> u64 {
        self.modified
    }

    fn get_depth(&self) -> usize {
        self.depth
    }
}
