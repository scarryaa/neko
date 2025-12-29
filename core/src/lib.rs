pub mod app;
pub mod commands;
pub mod config;
pub mod ffi;
pub mod file_system;
pub mod shortcuts;
pub mod tab;
#[cfg(test)]
pub mod test_utils;
pub mod text;
pub mod theme;

pub use app::AppState;
pub use commands::{
    Command, CommandResult, DocumentTarget, JumpCommand, LineTarget, UiIntent, execute_command,
    get_available_tab_commands, run_tab_command,
    tab::{TabCommandState, TabContext},
    tab_command_state,
};
pub use config::{Config, ConfigManager};
pub use file_system::{FileNode, FileTree};
pub use shortcuts::ShortcutsManager;
pub(crate) use tab::{Tab, TabManager, types::*};
pub use text::{
    AddCursorDirection, Buffer, ChangeSet, Cursor, CursorEntry, Editor, Selection, SelectionManager,
};
pub use theme::{Theme, ThemeManager};
