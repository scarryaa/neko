use crate::{
    AppState, Buffer, ConfigManager, FileTree, ShortcutsManager, ThemeManager,
    ffi::{
        AppController, CommandController, EditorController, FileTreeController, TabController,
        new_app_controller, wrappers::*,
    },
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
        pub document_id: u64,
        pub pinned: bool,
        pub scroll_offsets: ScrollOffsetFfi,
        pub title: String,
        pub path_present: bool,
        pub path: String,
        pub modified: bool,
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

    pub struct MoveActiveTabResult {
        pub id: u64,
        pub reopened: bool,
        pub snapshot: TabSnapshot,
    }

    // pub struct NewTabResult {
    //     pub id: u64,
    //     pub index: u32,
    //     pub snapshot: TabSnapshot,
    // }
    //
    // pub struct FileOpenResult {
    //     pub success: bool,
    //     pub snapshot: TabSnapshot,
    // }
    //
    // pub struct CloseTabResult {
    //     pub closed: bool,
    //     pub has_active: bool,
    //     pub active_id: u64,
    // }

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

    pub enum FileSystemErrorFfi {
        Io,
        MissingName,
        BadSystemTime,
    }

    #[derive(Debug)]
    pub enum DocumentErrorFfi {
        Io,
        InvalidId,
        NoPath,
        NotFound,
        ViewNotFound,
    }

    pub struct OpenTabResultFfi {
        pub tab_id: u64,
        pub found_tab_id: bool,
        pub tab_already_exists: bool,
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

    struct FileExplorerContextFfi {
        item_path: String,
        target_is_item: bool,
        item_is_directory: bool,
    }

    struct FileExplorerCommandStateFfi {
        can_make_new_file: bool,
        can_make_new_folder: bool,
        can_reveal_in_system: bool,
        can_open_in_terminal: bool,

        can_find_in_folder: bool,

        can_cut: bool,
        can_copy: bool,
        can_duplicate: bool,
        can_paste: bool,

        can_copy_path: bool,
        can_copy_relative_path: bool,

        can_show_history: bool,

        can_rename: bool,
        can_delete: bool,

        can_expand_item: bool,
        can_collapse_item: bool,
        can_collapse_all: bool,
    }

    struct FileExplorerCommandFfi {
        id: String,
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

    struct CreateDocumentTabAndViewResultFfi {
        document_id: u64,
        view_id: u64,
        tab_id: u64,
    }

    extern "Rust" {
        type AppState;
        type ConfigManager;
        type ShortcutsManager;
        type ThemeManager;
        type FileTree;
        type Buffer;
        type EditorController;
        type AppController;
        type TabController;
        type FileTreeController;
        type CommandController;

        // AppController
        pub fn new_app_controller(
            config_manager: &ConfigManager,
            root_path: String,
        ) -> Box<AppController>;
        pub fn editor_controller(self: &AppController) -> Box<EditorController>;
        pub fn tab_controller(self: &AppController) -> Box<TabController>;
        pub fn file_tree_controller(self: &AppController) -> Box<FileTreeController>;
        pub fn command_controller(self: &AppController) -> Box<CommandController>;

        pub(crate) fn save_document(self: &mut AppController, id: u64) -> bool;
        pub(crate) fn save_document_as(self: &mut AppController, id: u64, path: &str) -> bool;
        pub fn ensure_tab_for_path(
            self: &mut AppController,
            path: &str,
            add_to_history: bool,
        ) -> Result<OpenTabResultFfi>;

        // EditorController
        pub fn select_word(self: &mut EditorController, row: usize, column: usize) -> ChangeSetFfi;
        pub(crate) fn move_to(
            self: &mut EditorController,
            row: usize,
            col: usize,
            clear_selection: bool,
        ) -> ChangeSetFfi;
        pub(crate) fn select_to(
            self: &mut EditorController,
            row: usize,
            col: usize,
        ) -> ChangeSetFfi;
        pub(crate) fn move_left(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn move_right(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn move_up(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn move_down(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn insert_text(self: &mut EditorController, text: &str) -> ChangeSetFfi;
        pub(crate) fn insert_newline(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn insert_tab(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn backspace(self: &mut EditorController) -> ChangeSetFfi;
        #[cxx_name = "delete_forwards"]
        pub(crate) fn delete(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn select_all(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn select_left(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn select_right(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn select_up(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn select_down(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn clear_selection(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn undo(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn redo(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn get_max_width(self: &EditorController) -> f64;
        pub(crate) fn update_line_width(
            self: &mut EditorController,
            line_idx: usize,
            line_width: f64,
        );
        pub(crate) fn needs_width_measurement(self: &EditorController, line_idx: usize) -> bool;
        pub(crate) fn get_text(self: &EditorController) -> String;
        pub(crate) fn get_line(self: &EditorController, line_idx: usize) -> String;
        pub(crate) fn get_line_count(self: &EditorController) -> usize;
        pub(crate) fn get_cursor_positions(self: &EditorController) -> Vec<CursorPosition>;
        pub(crate) fn get_selection(self: &mut EditorController) -> Selection;
        pub(crate) fn copy(self: &EditorController) -> String;
        pub(crate) fn paste(self: &mut EditorController, text: &str) -> ChangeSetFfi;
        pub(crate) fn add_cursor(self: &mut EditorController, direction: AddCursorDirectionFfi);
        pub(crate) fn remove_cursor(self: &mut EditorController, row: usize, col: usize);
        pub(crate) fn cursor_exists_at(self: &EditorController, row: usize, col: usize) -> bool;
        pub(crate) fn clear_cursors(self: &mut EditorController) -> ChangeSetFfi;
        pub(crate) fn has_active_selection(self: &EditorController) -> bool;
        pub(crate) fn cursor_exists_at_row(self: &EditorController, row: usize) -> bool;
        pub(crate) fn active_cursor_index(self: &EditorController) -> usize;
        pub(crate) fn get_last_added_cursor(self: &EditorController) -> CursorPosition;
        pub(crate) fn number_of_selections(self: &EditorController) -> usize;
        pub(crate) fn line_length(self: &EditorController, row: usize) -> usize;
        pub(crate) fn lines(self: &EditorController) -> Vec<String>;
        #[allow(clippy::too_many_arguments)]
        pub(crate) fn select_word_drag(
            self: &mut EditorController,
            anchor_start_row: usize,
            anchor_start_col: usize,
            anchor_end_row: usize,
            anchor_end_col: usize,
            row: usize,
            col: usize,
        ) -> ChangeSetFfi;
        pub(crate) fn select_line_drag(
            self: &mut EditorController,
            anchor_row: usize,
            row: usize,
        ) -> ChangeSetFfi;
        pub(crate) fn buffer_is_empty(self: &EditorController) -> bool;

        // FileTreeController
        pub fn toggle_expanded(self: &mut FileTreeController, path: &str);
        pub(crate) fn set_root_path(self: &mut FileTreeController, path: &str) -> Result<()>;
        pub(crate) fn get_next_node(
            self: &FileTreeController,
            current_path: &str,
        ) -> Result<FileNodeSnapshot>;
        pub(crate) fn get_prev_node(
            self: &FileTreeController,
            current_path: &str,
        ) -> Result<FileNodeSnapshot>;
        pub(crate) fn get_path_of_parent(self: &FileTreeController, path: &str) -> String;
        pub(crate) fn ensure_path_visible(self: &mut FileTreeController, path: &str) -> Result<()>;
        pub fn set_expanded(self: &mut FileTreeController, path: &str);
        pub(crate) fn set_collapsed(self: &mut FileTreeController, path: &str);
        pub(crate) fn toggle_select_for_path(self: &mut FileTreeController, path: &str);
        pub(crate) fn set_current_path(self: &mut FileTreeController, path: &str);
        pub(crate) fn clear_current_path(self: &mut FileTreeController);
        pub(crate) fn get_children(
            self: &mut FileTreeController,
            path: &str,
        ) -> Result<Vec<FileNodeSnapshot>>;
        pub(crate) fn refresh_dir(self: &mut FileTreeController, path: &str) -> Result<()>;
        pub(crate) fn get_tree_snapshot(self: &FileTreeController) -> FileTreeSnapshot;

        // TabController
        pub fn set_active_tab(self: &mut TabController, id: u64) -> Result<()>;
        pub fn get_close_tab_ids(
            self: &TabController,
            operation_type: CloseTabOperationTypeFfi,
            anchor_tab_id: u64,
            close_pinned: bool,
        ) -> Vec<u64>;
        pub fn get_tab_snapshot(self: &TabController, id: u64) -> TabSnapshotMaybe;
        pub(crate) fn close_tabs(
            self: &TabController,
            operation_type: CloseTabOperationTypeFfi,
            anchor_tab_id: u64,
            close_pinned: bool,
        ) -> CloseManyTabsResult;
        pub fn get_tabs_snapshot(self: &TabController) -> TabsSnapshot;
        pub(crate) fn move_tab(self: &mut TabController, from: usize, to: usize) -> bool;
        // TODO(scarlet): Decouple this fn from the buffer arg if possible
        pub fn move_active_tab_by(
            self: &mut TabController,
            delta: i64,
            use_history: bool,
        ) -> MoveActiveTabResult;
        pub(crate) fn pin_tab(self: &mut TabController, id: u64) -> PinTabResult;
        pub(crate) fn unpin_tab(self: &mut TabController, id: u64) -> PinTabResult;
        pub fn create_document_tab_and_view(
            self: &mut TabController,
            title: String,
            add_tab_to_history: bool,
            activate_view: bool,
        ) -> CreateDocumentTabAndViewResultFfi;
        pub(crate) fn set_tab_scroll_offsets(
            self: &mut TabController,
            id: u64,
            new_offsets: ScrollOffsetFfi,
        ) -> bool;

        // Command Controller
        // Jump Commands
        pub fn execute_jump_command(self: &CommandController, cmd: JumpCommandFfi);
        pub fn execute_jump_key(self: &CommandController, key: String);
        pub fn get_available_jump_commands(self: &CommandController) -> Vec<JumpCommandFfi>;

        // Tab Commands
        pub fn run_tab_command(
            self: &CommandController,
            id: &str,
            ctx: TabContextFfi,
            close_pinned: bool,
        ) -> bool;
        pub fn get_tab_command_state(self: &CommandController, id: u64) -> TabCommandStateFfi;
        pub fn get_available_tab_commands(self: &CommandController) -> Vec<TabCommandFfi>;

        // File Explorer Commands
        pub fn run_file_explorer_command(
            self: &CommandController,
            id: &str,
            ctx: FileExplorerContextFfi,
        ) -> bool;
        pub fn get_file_explorer_command_state(
            self: &CommandController,
            id: u64,
        ) -> FileExplorerCommandStateFfi;
        pub fn get_available_file_explorer_commands(
            self: &CommandController,
        ) -> Vec<FileExplorerCommandFfi>;

        // General Commands
        pub(crate) fn new_command(
            self: &CommandController,
            key: String,
            display_name: String,
            kind: CommandKindFfi,
            argument: String,
        ) -> CommandFfi;
        pub fn execute_command(
            self: &CommandController,
            cmd: CommandFfi,
            config: &mut ConfigManager,
            theme: &mut ThemeManager,
        ) -> CommandResultFfi;
        pub fn get_available_commands(self: &CommandController) -> Vec<CommandFfi>;

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
    }
}
