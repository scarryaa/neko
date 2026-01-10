pub mod app;
pub mod commands;
pub mod config;
mod ffi;
pub mod file_system;
pub mod shortcuts;
pub mod tab;
#[cfg(test)]
pub mod test_utils;
pub mod text;
pub mod theme;

pub use app::{AppState, FileIoManager, FileOperationsManager};
pub use commands::{
    Command, CommandResult, DocumentTarget, FileExplorerCommand, FileExplorerCommandResult,
    FileExplorerNavigationDirection, FileExplorerUiIntent, JumpAliasInfo, JumpCommand, JumpHistory,
    JumpManagementCommand, JumpManagementResult, LineTarget, TabCommand, TabCommandState,
    TabContext, UiIntent, execute_command, execute_jump_command, execute_jump_key,
    execute_jump_management_command, file_explorer_command_state, get_available_commands,
    get_available_file_explorer_commands, get_available_jump_commands, get_available_tab_commands,
    run_file_explorer_command, run_tab_command, tab_command_state,
};
pub use config::{Config, ConfigManager};
pub use file_system::{FileNode, FileTree, error::*, result::*};
pub use shortcuts::{Shortcut, ShortcutsManager};
pub use tab::{Tab, TabManager, error::*, types::*};
use text::CursorManager;
pub use text::{
    AddCursorDirection, Buffer, Change, ChangeSet, Cursor, CursorEntry, Document, DocumentId,
    DocumentManager, DocumentResult, Editor, Selection, SelectionManager, View, ViewId,
    ViewManager, Viewport, document::error::*, view::error::*,
};
pub use theme::{Theme, ThemeManager};
