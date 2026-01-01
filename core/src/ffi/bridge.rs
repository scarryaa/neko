use crate::{
    AppState, Buffer, ConfigManager, Editor, FileTree, ShortcutsManager, ThemeManager,
    execute_jump_key, ffi::wrappers::*,
};

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

    #[derive(Default, Clone)]
    struct ScrollOffsetFfi {
        x: i32,
        y: i32,
    }

    struct ChangeSetFfi {
        mask: u32,
        line_count_before: u32,
        line_count_after: u32,
        dirty_first_row: i32,
        dirty_last_row: i32,
    }

    enum CloseTabOperationTypeFfi {
        Single,
        Left,
        Right,
        Others,
        Clean,
        All,
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
        OpenConfig,
        JumpManagement,
        NoOp,
    }

    struct CommandFfi {
        key: String,
        display_name: String,
        kind: CommandKindFfi,
        argument: String,
    }

    enum UiIntentKindFfi {
        ToggleFileExplorer,
        ApplyTheme,
        OpenConfig,
        ShowJumpAliases,
    }

    // TODO(scarlet): Figure out a better system than adding a new field for each type
    struct UiIntentFfi {
        pub kind: UiIntentKindFfi,
        pub argument_str: String,
        pub argument_u64: u64,
        pub jump_aliases: Vec<JumpAliasInfoFfi>,
    }

    struct CommandResultFfi {
        pub intents: Vec<UiIntentFfi>,
    }

    #[derive(Debug)]
    pub enum JumpCommandKindFfi {
        ToPosition,
        ToLine,
        ToDocument,
        ToLastTarget,
    }

    #[derive(Debug)]
    enum LineTargetFfi {
        Start,
        Middle,
        End,
    }

    #[derive(Debug)]
    enum DocumentTargetFfi {
        Start,
        Middle,
        End,
        Quarter,
        ThreeQuarters,
    }

    #[derive(Debug, Clone)]
    pub struct JumpCommandFfi {
        pub key: String,
        pub display_name: String,
        pub kind: JumpCommandKindFfi,
        pub row: i64,
        pub column: i64,
        pub line_target: LineTargetFfi,
        pub document_target: DocumentTargetFfi,
    }

    #[derive(Debug)]
    struct JumpAliasInfoFfi {
        pub name: String,
        pub spec: String,
        pub description: String,
        pub is_builtin: bool,
    }

    struct EditorConfigFfi {
        font_size: u32,
        font_family: String,
    }

    struct FileExplorerConfigFfi {
        font_size: u32,
        font_family: String,
        directory_present: bool,
        directory: String,
        shown: bool,
        width: u32,
        right: bool,
    }

    struct InterfaceConfigFfi {
        font_size: u32,
        font_family: String,
    }

    struct TerminalConfigFfi {
        font_size: u32,
        font_family: String,
    }

    struct ConfigSnapshotFfi {
        editor: EditorConfigFfi,
        file_explorer: FileExplorerConfigFfi,
        interface: InterfaceConfigFfi,
        terminal: TerminalConfigFfi,
        current_theme: String,
    }

    #[derive(Default, Clone)]
    struct TabSnapshot {
        pub id: u64,
        pub title: String,
        pub path_present: bool,
        pub path: String,
        pub modified: bool,
        pub pinned: bool,
        pub scroll_offsets: ScrollOffsetFfi,
    }

    struct TabsSnapshot {
        pub active_present: bool,
        pub active_id: u64,
        pub tabs: Vec<TabSnapshot>,
    }

    pub struct TabSnapshotMaybe {
        pub found: bool,
        pub snapshot: TabSnapshot,
    }

    pub struct NewTabResult {
        pub id: u64,
        pub index: u32,
        pub snapshot: TabSnapshot,
    }

    pub struct MoveActiveTabResult {
        pub id: u64,
        pub reopened: bool,
        pub snapshot: TabSnapshot,
    }

    pub struct FileOpenResult {
        pub success: bool,
        pub snapshot: TabSnapshot,
    }

    pub struct CloseTabResult {
        pub closed: bool,
        pub has_active: bool,
        pub active_id: u64,
    }

    pub struct CloseManyTabsResult {
        pub success: bool,
        pub closed_ids: Vec<u64>,
        pub has_active: bool,
        pub active_id: u64,
    }

    pub struct PinTabResult {
        pub success: bool,
        pub from_index: u32,
        pub to_index: u32,
        pub snapshot: TabSnapshot,
    }

    enum FileSystemErrorFfi {
        Io,
        MissingName,
        BadSystemTime,
    }

    struct FileNodeSnapshot {
        path: String,
        name: String,
        is_dir: bool,
        is_hidden: bool,
        is_expanded: bool,
        is_selected: bool,
        is_current: bool,
        size: u64,
        modified: u64,
        depth: usize,
    }

    struct FileTreeSnapshot {
        pub root_present: bool,
        pub root: String,
        pub nodes: Vec<FileNodeSnapshot>,
    }

    struct TabContextFfi {
        id: u64,
        is_pinned: bool,
        is_modified: bool,
        file_path_present: bool,
        file_path: String,
    }

    struct TabCommandStateFfi {
        can_close: bool,
        can_close_others: bool,
        can_close_left: bool,
        can_close_right: bool,
        can_close_all: bool,
        can_close_clean: bool,
        can_copy_path: bool,
        can_reveal: bool,
        is_pinned: bool,
    }

    struct TabCommandFfi {
        id: String,
    }

    extern "Rust" {
        type AppState;
        type ConfigManager;
        type ShortcutsManager;
        type ThemeManager;
        type Editor;
        type FileTree;
        type Buffer;

        // AppState
        fn new_app_state(root_path: &str, config_manager: &ConfigManager) -> Result<Box<AppState>>;
        #[cxx_name = "active_editor"]
        pub(crate) fn active_editor_wrapper(self: &AppState) -> &Editor;
        #[cxx_name = "active_editor_mut"]
        pub(crate) fn active_editor_mut_wrapper(self: &mut AppState) -> &mut Editor;
        pub(crate) fn get_file_tree(self: &AppState) -> &FileTree;
        pub(crate) fn get_file_tree_mut(self: &mut AppState) -> &mut FileTree;
        #[cxx_name = "get_close_tab_ids"]
        pub(crate) fn get_close_tab_ids_wrapper(
            self: &mut AppState,
            operation_type: CloseTabOperationTypeFfi,
            anchor_id: u64,
            close_pinned: bool,
        ) -> Vec<u64>;
        pub(crate) fn get_tabs_snapshot(self: &AppState) -> TabsSnapshot;
        pub(crate) fn get_tab_snapshot(self: &AppState, id: u64) -> TabSnapshotMaybe;

        #[cxx_name = "new_tab"]
        pub(crate) fn new_tab_wrapper(self: &mut AppState) -> NewTabResult;
        #[cxx_name = "close_tabs"]
        pub(crate) fn close_tabs_wrapper(
            self: &mut AppState,
            operation_type: CloseTabOperationTypeFfi,
            anchor_tab_id: u64,
            close_pinned: bool,
        ) -> CloseManyTabsResult;
        #[cxx_name = "set_active_tab"]
        pub(crate) fn set_active_tab_wrapper(self: &mut AppState, id: u64) -> Result<()>;
        #[cxx_name = "move_tab"]
        pub(crate) fn move_tab_wrapper(self: &mut AppState, from: usize, to: usize) -> bool;
        #[cxx_name = "open_file"]
        pub(crate) fn open_file_wrapper(self: &mut AppState, path: &str) -> FileOpenResult;
        #[cxx_name = "move_active_tab_by"]
        pub fn move_active_tab_by_wrapper(
            self: &mut AppState,
            delta: i64,
            use_history: bool,
        ) -> MoveActiveTabResult;
        #[cxx_name = "pin_tab"]
        pub(crate) fn pin_tab_wrapper(self: &mut AppState, id: u64) -> PinTabResult;
        #[cxx_name = "unpin_tab"]
        pub(crate) fn unpin_tab_wrapper(self: &mut AppState, id: u64) -> PinTabResult;
        // #[cxx_name = "save_tab"]
        // pub(crate) fn save_tab_wrapper(self: &mut AppState, id: u64) -> bool;
        // #[cxx_name = "save_tab_as"]
        // pub(crate) fn save_tab_as_wrapper(self: &mut AppState, id: u64, path: &str) -> bool;
        #[cxx_name = "set_tab_scroll_offsets"]
        pub(crate) fn set_tab_scroll_offsets_wrapper(
            self: &mut AppState,
            id: u64,
            new_offsets: ScrollOffsetFfi,
        ) -> bool;

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
            buffer: &mut Buffer,
            row: usize,
            col: usize,
            clear_selection: bool,
        ) -> ChangeSetFfi;
        #[cxx_name = "select_to"]
        pub(crate) fn select_to_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
            row: usize,
            col: usize,
        ) -> ChangeSetFfi;
        #[cxx_name = "move_left"]
        pub(crate) fn move_left_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "move_right"]
        pub(crate) fn move_right_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "move_up"]
        pub(crate) fn move_up_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "move_down"]
        pub(crate) fn move_down_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "insert_text"]
        pub(crate) fn insert_text_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
            text: &str,
        ) -> ChangeSetFfi;
        #[cxx_name = "insert_newline"]
        pub(crate) fn insert_newline_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
        ) -> ChangeSetFfi;
        #[cxx_name = "insert_tab"]
        pub(crate) fn insert_tab_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "backspace"]
        pub(crate) fn backspace_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "delete_forwards"]
        pub(crate) fn delete_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "select_all"]
        pub(crate) fn select_all_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "select_left"]
        pub(crate) fn select_left_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "select_right"]
        pub(crate) fn select_right_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "select_up"]
        pub(crate) fn select_up_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "select_down"]
        pub(crate) fn select_down_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "clear_selection"]
        pub(crate) fn clear_selection_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
        ) -> ChangeSetFfi;
        #[cxx_name = "undo"]
        pub(crate) fn undo_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        #[cxx_name = "redo"]
        pub(crate) fn redo_wrapper(self: &mut Editor, buffer: &mut Buffer) -> ChangeSetFfi;
        pub(crate) fn get_max_width(self: &Editor) -> f64;
        #[cxx_name = "set_line_width"]
        pub(crate) fn update_line_width(self: &mut Editor, line_idx: usize, line_width: f64);
        pub(crate) fn needs_width_measurement(self: &Editor, line_idx: usize) -> bool;
        pub(crate) fn get_text(self: &Editor, buffer: &Buffer) -> String;
        pub(crate) fn get_line(self: &Editor, buffer: &Buffer, line_idx: usize) -> String;
        pub(crate) fn get_line_count(self: &Editor, buffer: &Buffer) -> usize;
        pub(crate) fn get_cursor_positions(self: &Editor) -> Vec<CursorPosition>;
        pub(crate) fn get_selection(self: &mut Editor) -> Selection;
        pub(crate) fn copy(self: &Editor, buffer: &Buffer) -> String;
        pub(crate) fn paste(self: &mut Editor, buffer: &mut Buffer, text: &str) -> ChangeSetFfi;
        #[cxx_name = "add_cursor"]
        pub(crate) fn add_cursor_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
            direction: AddCursorDirectionFfi,
        );
        pub(crate) fn remove_cursor(self: &mut Editor, row: usize, col: usize);
        pub(crate) fn cursor_exists_at(self: &Editor, row: usize, col: usize) -> bool;
        #[cxx_name = "clear_cursors"]
        pub(crate) fn clear_cursors_wrapper(self: &mut Editor, buffer: &mut Buffer)
        -> ChangeSetFfi;
        pub(crate) fn has_active_selection(self: &Editor) -> bool;
        pub(crate) fn cursor_exists_at_row(self: &Editor, row: usize) -> bool;
        #[cxx_name = "get_active_cursor_index"]
        pub(crate) fn active_cursor_index(self: &Editor) -> usize;
        pub(crate) fn get_last_added_cursor(self: &Editor) -> CursorPosition;
        #[cxx_name = "get_number_of_selections"]
        pub(crate) fn number_of_selections(self: &Editor) -> usize;
        #[cxx_name = "get_line_length"]
        pub(crate) fn line_length(self: &Editor, buffer: &Buffer, row: usize) -> usize;
        #[cxx_name = "get_lines"]
        pub(crate) fn lines(self: &Editor, buffer: &Buffer) -> Vec<String>;
        #[cxx_name = "select_word"]
        pub(crate) fn select_word_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
            row: usize,
            col: usize,
        ) -> ChangeSetFfi;
        #[cxx_name = "select_word_drag"]
        pub(crate) fn select_word_drag_wrapper(
            self: &mut Editor,
            buffer: &mut Buffer,
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
            buffer: &mut Buffer,
            anchor_row: usize,
            row: usize,
        ) -> ChangeSetFfi;
        pub(crate) fn buffer_is_empty(self: &Editor, buffer: &Buffer) -> bool;

        // FileTree
        #[cxx_name = "set_root_dir"]
        pub(crate) fn set_root_path(self: &mut FileTree, path: &str) -> Result<()>;
        pub(crate) fn get_next_node_wrapper(
            self: &FileTree,
            current_path: &str,
        ) -> Result<FileNodeSnapshot>;
        pub(crate) fn get_prev_node_wrapper(
            self: &FileTree,
            current_path: &str,
        ) -> Result<FileNodeSnapshot>;
        pub(crate) fn get_path_of_parent(self: &FileTree, path: &str) -> String;
        pub(crate) fn toggle_expanded(self: &mut FileTree, path: &str);
        pub(crate) fn ensure_path_visible(self: &mut FileTree, path: &str) -> Result<()>;
        pub(crate) fn set_collapsed(self: &mut FileTree, path: &str);
        pub(crate) fn toggle_select_for_path(self: &mut FileTree, path: &str);
        pub(crate) fn set_current_path(self: &mut FileTree, path: &str);
        pub(crate) fn clear_current_path(self: &mut FileTree);
        #[cxx_name = "get_children"]
        pub(crate) fn get_children_wrapper(
            self: &mut FileTree,
            path: &str,
        ) -> Result<Vec<FileNodeSnapshot>>;
        pub(crate) fn refresh_dir(self: &mut FileTree, path: &str) -> Result<()>;
        pub(crate) fn get_tree_snapshot(self: &FileTree) -> FileTreeSnapshot;

        // Commands
        #[cxx_name = "execute_command"]
        pub(crate) fn execute_command_wrapper(
            cmd: CommandFfi,
            config: &mut ConfigManager,
            theme: &mut ThemeManager,
            app: &mut AppState,
        ) -> CommandResultFfi;
        pub(crate) fn new_command(
            key: String,
            display_name: String,
            kind: CommandKindFfi,
            argument: String,
        ) -> CommandFfi;
        #[cxx_name = "get_available_commands"]
        pub(crate) fn get_available_commands_wrapper() -> Vec<CommandFfi>;

        // Jump commands
        #[cxx_name = "execute_jump_command"]
        pub(crate) fn execute_jump_command_wrapper(cmd: JumpCommandFfi, app_state: &mut AppState);
        pub(crate) fn execute_jump_key(key: String, app_state: &mut AppState);
        #[cxx_name = "get_available_jump_commands"]
        pub(crate) fn get_available_jump_commands_wrapper() -> Vec<JumpCommandFfi>;

        // Tab commands
        pub(crate) fn get_tab_command_state(app_state: &AppState, id: u64) -> TabCommandStateFfi;
        #[cxx_name = "run_tab_command"]
        pub(crate) fn run_tab_command_wrapper(
            app_state: &mut AppState,
            id: &str,
            ctx: TabContextFfi,
            close_pinned: bool,
        ) -> bool;
        #[cxx_name = "get_available_tab_commands"]
        pub(crate) fn get_available_tab_commands_wrapper() -> Vec<TabCommandFfi>;
    }
}
