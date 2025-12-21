use crate::AppState;
use crate::Editor;
use crate::FileTree;
use crate::ShortcutsManager;
use crate::config::ConfigManager;
use crate::ffi::wrappers::*;
use crate::theme::ThemeManager;

#[cxx::bridge(namespace = "neko")]
pub mod ffi {
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

    struct Shortcut {
        key: String,
        key_combo: String,
    }

    enum CommandKindFfi {
        FileExplorerToggle,
        ChangeTheme,
    }

    struct CommandFfi {
        kind: CommandKindFfi,
        argument: String,
    }

    enum UiIntentKindFfi {
        ToggleFileExplorer,
        ApplyTheme,
    }

    struct UiIntentFfi {
        pub kind: UiIntentKindFfi,
        pub argument: String,
    }

    struct CommandResultFfi {
        pub intents: Vec<UiIntentFfi>,
    }

    struct ConfigSnapshotFfi {
        editor_font_size: u32,
        editor_font_family: String,

        file_explorer_font_size: u32,
        file_explorer_font_family: String,
        file_explorer_directory_present: bool,
        file_explorer_directory: String,
        file_explorer_shown: bool,
        file_explorer_width: u32,
        file_explorer_right: bool,

        interface_font_family: String,
        interface_font_size: u32,
        terminal_font_family: String,
        terminal_font_size: u32,

        current_theme: String,
    }

    extern "Rust" {
        type AppState;
        type ConfigManager;
        type ShortcutsManager;
        type ThemeManager;
        type Editor;
        type FileTree;

        // AppState
        pub(crate) fn new_app_state(root_path: &str) -> Result<Box<AppState>>;
        pub(crate) fn get_tab_count(self: &AppState) -> usize;
        pub(crate) fn get_tabs_empty(self: &AppState) -> bool;
        #[cxx_name = "get_active_editor"]
        pub(crate) fn get_active_editor_wrapper(self: &AppState) -> &Editor;
        #[cxx_name = "get_active_editor_mut"]
        pub(crate) fn get_active_editor_mut_wrapper(self: &mut AppState) -> &mut Editor;
        #[cxx_name = "get_active_tab_path"]
        pub(crate) fn get_active_tab_path_wrapper(self: &AppState) -> String;
        pub(crate) fn get_active_tab_index(self: &AppState) -> usize;
        pub(crate) fn get_tab_titles(self: &AppState) -> Vec<String>;
        pub(crate) fn get_tab_modified_states(self: &AppState) -> Vec<bool>;
        pub(crate) fn get_tab_modified(self: &AppState, index: usize) -> bool;
        pub(crate) fn get_tab_pinned_states(self: &AppState) -> Vec<bool>;
        #[cxx_name = "get_tab_index_by_path"]
        pub(crate) fn get_tab_index_by_path_wrapper(self: &AppState, path: &str) -> i64;
        pub(crate) fn tab_with_path_exists(self: &AppState, path: &str) -> bool;
        pub(crate) fn get_file_tree(self: &AppState) -> &FileTree;
        pub(crate) fn get_file_tree_mut(self: &mut AppState) -> &mut FileTree;
        #[cxx_name = "get_tab_path"]
        pub(crate) fn get_tab_path_wrapper(self: &AppState, index: usize) -> String;

        pub(crate) fn new_tab(self: &mut AppState) -> usize;
        #[cxx_name = "close_tab"]
        pub(crate) fn close_tab_wrapper(self: &mut AppState, index: usize) -> bool;
        #[cxx_name = "close_other_tabs"]
        pub(crate) fn close_other_tabs_wrapper(self: &mut AppState, index: usize) -> bool;
        #[cxx_name = "close_left_tabs"]
        pub(crate) fn close_left_tabs_wrapper(self: &mut AppState, index: usize) -> bool;
        #[cxx_name = "close_right_tabs"]
        pub(crate) fn close_right_tabs_wrapper(self: &mut AppState, index: usize) -> bool;
        pub(crate) fn set_active_tab_index(self: &mut AppState, index: usize) -> Result<()>;
        #[cxx_name = "move_tab"]
        pub(crate) fn move_tab_wrapper(self: &mut AppState, from: usize, to: usize) -> bool;
        #[cxx_name = "open_file"]
        pub(crate) fn open_file_wrapper(self: &mut AppState, path: &str) -> bool;
        #[cxx_name = "save_active_tab"]
        pub(crate) fn save_active_tab_wrapper(self: &mut AppState) -> bool;
        #[cxx_name = "save_active_tab_and_set_path"]
        pub(crate) fn save_active_tab_and_set_path_wrapper(self: &mut AppState, path: &str)
        -> bool;
        #[cxx_name = "pin_tab"]
        pub(crate) fn pin_tab_wrapper(self: &mut AppState, index: usize) -> bool;
        #[cxx_name = "unpin_tab"]
        pub(crate) fn unpin_tab_wrapper(self: &mut AppState, index: usize) -> bool;

        // ConfigManager
        pub(crate) fn new_config_manager() -> Result<Box<ConfigManager>>;
        #[cxx_name = "get_config_snapshot"]
        pub(crate) fn get_snapshot_wrapper(self: &ConfigManager) -> ConfigSnapshotFfi;
        #[cxx_name = "apply_config_snapshot"]
        pub(crate) fn apply_snapshot_wrapper(self: &ConfigManager, snap: ConfigSnapshotFfi);
        #[cxx_name = "save_config"]
        pub(crate) fn save_config_wrapper(self: &mut ConfigManager) -> bool;
        #[cxx_name = "get_config_path"]
        pub(crate) fn get_config_path_wrapper(self: &ConfigManager) -> String;

        // ShortcutsManager
        pub(crate) fn new_shortcuts_manager() -> Result<Box<ShortcutsManager>>;
        #[cxx_name = "get_shortcut"]
        pub(crate) fn get_shortcut_wrapper(self: &ShortcutsManager, key: String) -> Shortcut;
        #[cxx_name = "get_shortcuts"]
        pub(crate) fn get_shortcuts_wrapper(self: &ShortcutsManager) -> Vec<Shortcut>;
        #[cxx_name = "add_shortcuts"]
        pub(crate) fn add_shortcuts_wrapper(self: &ShortcutsManager, shortcuts: Vec<Shortcut>);

        // ThemeManager
        pub(crate) fn new_theme_manager() -> Result<Box<ThemeManager>>;
        #[cxx_name = "get_color"]
        pub(crate) fn get_color_wrapper(self: &mut ThemeManager, key: &str) -> String;
        pub(crate) fn set_theme(self: &mut ThemeManager, theme_name: &str) -> bool;
        #[cxx_name = "get_current_theme_name"]
        pub(crate) fn get_current_theme_name_wrapper(self: &ThemeManager) -> String;

        // Editor
        #[cxx_name = "move_to"]
        pub(crate) fn move_to_wrapper(
            self: &mut Editor,
            row: usize,
            col: usize,
            clear_selection: bool,
        ) -> ChangeSetFfi;
        #[cxx_name = "select_to"]
        pub(crate) fn select_to_wrapper(self: &mut Editor, row: usize, col: usize) -> ChangeSetFfi;
        #[cxx_name = "move_left"]
        pub(crate) fn move_left_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "move_right"]
        pub(crate) fn move_right_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "move_up"]
        pub(crate) fn move_up_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "move_down"]
        pub(crate) fn move_down_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "insert_text"]
        pub(crate) fn insert_text_wrapper(self: &mut Editor, text: &str) -> ChangeSetFfi;
        #[cxx_name = "insert_newline"]
        pub(crate) fn insert_newline_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "insert_tab"]
        pub(crate) fn insert_tab_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "backspace"]
        pub(crate) fn backspace_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "delete_forwards"]
        pub(crate) fn delete_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_all"]
        pub(crate) fn select_all_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_left"]
        pub(crate) fn select_left_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_right"]
        pub(crate) fn select_right_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_up"]
        pub(crate) fn select_up_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "select_down"]
        pub(crate) fn select_down_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "clear_selection"]
        pub(crate) fn clear_selection_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "undo"]
        pub(crate) fn undo_wrapper(self: &mut Editor) -> ChangeSetFfi;
        #[cxx_name = "redo"]
        pub(crate) fn redo_wrapper(self: &mut Editor) -> ChangeSetFfi;
        pub(crate) fn get_max_width(self: &Editor) -> f64;
        #[cxx_name = "set_line_width"]
        pub(crate) fn update_line_width(self: &mut Editor, line_idx: usize, line_width: f64);
        pub(crate) fn needs_width_measurement(self: &Editor, line_idx: usize) -> bool;
        pub(crate) fn get_text(self: &Editor) -> String;
        pub(crate) fn get_line(self: &Editor, line_idx: usize) -> String;
        pub(crate) fn get_line_count(self: &Editor) -> usize;
        pub(crate) fn get_cursor_positions(self: &Editor) -> Vec<CursorPosition>;
        pub(crate) fn get_selection(self: &mut Editor) -> Selection;
        pub(crate) fn copy(self: &Editor) -> String;
        pub(crate) fn paste(self: &mut Editor, text: &str) -> ChangeSetFfi;
        #[cxx_name = "add_cursor"]
        pub(crate) fn add_cursor_wrapper(self: &mut Editor, direction: AddCursorDirectionFfi);
        pub(crate) fn remove_cursor(self: &mut Editor, row: usize, col: usize);
        pub(crate) fn cursor_exists_at(self: &Editor, row: usize, col: usize) -> bool;
        #[cxx_name = "clear_cursors"]
        pub(crate) fn clear_cursors_wrapper(self: &mut Editor) -> ChangeSetFfi;
        pub(crate) fn has_active_selection(self: &Editor) -> bool;
        pub(crate) fn cursor_exists_at_row(self: &Editor, row: usize) -> bool;
        #[cxx_name = "get_active_cursor_index"]
        pub(crate) fn active_cursor_index(self: &Editor) -> usize;
        pub(crate) fn get_last_added_cursor(self: &Editor) -> CursorPosition;
        #[cxx_name = "get_number_of_selections"]
        pub(crate) fn number_of_selections(self: &Editor) -> usize;
        #[cxx_name = "get_line_length"]
        pub(crate) fn line_length(self: &Editor, row: usize) -> usize;
        #[cxx_name = "get_lines"]
        pub(crate) fn lines(self: &Editor) -> Vec<String>;
        #[cxx_name = "select_word"]
        pub(crate) fn select_word_wrapper(
            self: &mut Editor,
            row: usize,
            col: usize,
        ) -> ChangeSetFfi;
        #[cxx_name = "select_word_drag"]
        pub(crate) fn select_word_drag_wrapper(
            self: &mut Editor,
            anchor_start_row: usize,
            anchor_start_col: usize,
            anchor_end_row: usize,
            anchor_end_col: usize,
            row: usize,
            col: usize,
        ) -> ChangeSetFfi;
        #[cxx_name = "select_line_drag"]
        pub(crate) fn select_line_drag_wrapper(
            self: &mut Editor,
            anchor_row: usize,
            row: usize,
        ) -> ChangeSetFfi;
        pub(crate) fn buffer_is_empty(self: &Editor) -> bool;

        // FileTree
        #[cxx_name = "set_root_dir"]
        pub(crate) fn set_root_path(self: &mut FileTree, path: &str);
        #[cxx_name = "get_children"]
        pub(crate) fn get_children_wrapper(self: &mut FileTree, path: &str) -> Vec<FileNode>;
        #[cxx_name = "get_node"]
        pub(crate) fn get_node_wrapper(self: &FileTree, path: &str) -> FileNode;
        pub(crate) fn get_next_node(self: &FileTree, current_path: &str) -> FileNode;
        pub(crate) fn get_prev_node(self: &FileTree, current_path: &str) -> FileNode;
        pub(crate) fn get_path_of_parent(self: &FileTree, path: &str) -> String;
        pub(crate) fn toggle_expanded(self: &mut FileTree, path: &str);
        pub(crate) fn set_expanded(self: &mut FileTree, path: &str);
        pub(crate) fn set_collapsed(self: &mut FileTree, path: &str);
        #[cxx_name = "get_visible_nodes"]
        pub(crate) fn get_visible_nodes_wrapper(self: &mut FileTree) -> Vec<FileNode>;
        pub(crate) fn toggle_select(self: &mut FileTree, path: &str);
        pub(crate) fn set_current(self: &mut FileTree, path: &str);
        pub(crate) fn clear_current(self: &mut FileTree);
        #[cxx_name = "get_current_index"]
        pub(crate) fn get_index(self: &FileTree) -> usize;
        pub(crate) fn get_path_of_current(self: &FileTree) -> String;
        pub(crate) fn is_expanded(self: &FileTree, path: &str) -> bool;
        pub(crate) fn is_selected(self: &FileTree, path: &str) -> bool;
        pub(crate) fn is_current(self: &FileTree, path: &str) -> bool;
        pub(crate) fn refresh_dir(self: &mut FileTree, path: &str);

        // FileNode
        pub(crate) fn get_path(self: &FileNode) -> String;
        pub(crate) fn get_name(self: &FileNode) -> String;
        pub(crate) fn get_is_dir(self: &FileNode) -> bool;
        pub(crate) fn get_is_hidden(self: &FileNode) -> bool;
        pub(crate) fn get_size(self: &FileNode) -> u64;
        pub(crate) fn get_modified(self: &FileNode) -> u64;
        pub(crate) fn get_depth(self: &FileNode) -> usize;

        // Commands
        #[cxx_name = "execute_command"]
        pub(crate) fn execute_command_wrapper(
            cmd: CommandFfi,
            config: &mut ConfigManager,
            theme: &mut ThemeManager,
        ) -> CommandResultFfi;
        pub(crate) fn new_command(kind: CommandKindFfi, argument: String) -> CommandFfi;
    }
}
