use std::path::PathBuf;

use super::bridge::ffi::*;
use crate::{
    Command, CommandResult, Config, Cursor, UiIntent,
    commands::tab::{TabCommandState, TabContext},
    text::{ChangeSet, cursor_manager::AddCursorDirection},
};

impl From<Config> for ConfigSnapshotFfi {
    fn from(c: Config) -> Self {
        let (dir_present, dir) = match c.file_explorer_directory {
            Some(d) => (true, d),
            None => (false, String::new()),
        };

        Self {
            editor_font_size: c.editor_font_size as u32,
            editor_font_family: c.editor_font_family,

            file_explorer_font_size: c.file_explorer_font_size as u32,
            file_explorer_font_family: c.file_explorer_font_family,
            file_explorer_directory_present: dir_present,
            file_explorer_directory: dir,
            file_explorer_shown: c.file_explorer_shown,
            file_explorer_width: c.file_explorer_width as u32,
            file_explorer_right: c.file_explorer_right,

            terminal_font_family: c.terminal_font_family,
            terminal_font_size: c.terminal_font_size as u32,
            interface_font_family: c.interface_font_family,
            interface_font_size: c.interface_font_size as u32,

            current_theme: c.current_theme,
        }
    }
}

impl From<(i32, i32)> for ScrollOffsetFfi {
    fn from(offset: (i32, i32)) -> Self {
        ScrollOffsetFfi {
            x: offset.0,
            y: offset.1,
        }
    }
}

impl From<ChangeSet> for ChangeSetFfi {
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

impl From<Cursor> for CursorPosition {
    fn from(c: Cursor) -> Self {
        Self {
            row: c.row,
            col: c.column,
        }
    }
}

impl From<crate::shortcuts::Shortcut> for Shortcut {
    fn from(s: crate::shortcuts::Shortcut) -> Self {
        Self {
            key: s.key,
            key_combo: s.key_combo,
        }
    }
}

impl From<UiIntentFfi> for UiIntent {
    fn from(intent: UiIntentFfi) -> Self {
        match intent.kind {
            UiIntentKindFfi::ToggleFileExplorer => UiIntent::ToggleFileExplorer,
            UiIntentKindFfi::ApplyTheme => UiIntent::ApplyTheme {
                name: intent.argument,
            },
            // Should not happen
            _ => UiIntent::ToggleFileExplorer,
        }
    }
}

impl From<UiIntent> for UiIntentFfi {
    fn from(intent: UiIntent) -> Self {
        match intent {
            UiIntent::ToggleFileExplorer => UiIntentFfi {
                kind: UiIntentKindFfi::ToggleFileExplorer,
                argument: String::new(),
            },
            UiIntent::ApplyTheme { name } => UiIntentFfi {
                kind: UiIntentKindFfi::ApplyTheme,
                argument: name,
            },
        }
    }
}

impl From<CommandFfi> for Command {
    fn from(command: CommandFfi) -> Self {
        match command.kind {
            CommandKindFfi::FileExplorerToggle => Command::FileExplorerToggle,
            CommandKindFfi::ChangeTheme => Command::ChangeTheme(command.argument),
            // Should not happen
            _ => Command::FileExplorerToggle,
        }
    }
}

impl From<Command> for CommandFfi {
    fn from(command: Command) -> Self {
        match command {
            Command::FileExplorerToggle => CommandFfi {
                kind: CommandKindFfi::FileExplorerToggle,
                argument: String::new(),
            },
            Command::ChangeTheme(t) => CommandFfi {
                kind: CommandKindFfi::ChangeTheme,
                argument: t,
            },
        }
    }
}

impl From<CommandResult> for CommandResultFfi {
    fn from(command_result: CommandResult) -> Self {
        CommandResultFfi {
            intents: command_result
                .intents
                .into_iter()
                .map(UiIntentFfi::from)
                .collect(),
        }
    }
}

impl From<CommandResultFfi> for CommandResult {
    fn from(command_result: CommandResultFfi) -> Self {
        CommandResult {
            intents: command_result
                .intents
                .into_iter()
                .map(UiIntent::from)
                .collect(),
        }
    }
}

impl From<TabCommandState> for TabCommandStateFfi {
    fn from(command_state: TabCommandState) -> Self {
        Self {
            can_close: command_state.can_close,
            can_close_others: command_state.can_close_others,
            can_close_left: command_state.can_close_left,
            can_close_right: command_state.can_close_right,
            can_close_all: command_state.can_close_all,
            can_close_clean: command_state.can_close_clean,
            can_copy_path: command_state.can_copy_path,
            can_reveal: command_state.can_reveal,
            is_pinned: command_state.is_pinned,
        }
    }
}

impl From<TabContextFfi> for TabContext {
    fn from(ctx: TabContextFfi) -> Self {
        TabContext {
            id: ctx.id as usize,
            is_pinned: ctx.is_pinned,
            is_modified: ctx.is_modified,
            file_path: if ctx.file_path_present && !ctx.file_path.is_empty() {
                Some(PathBuf::from(ctx.file_path))
            } else {
                None
            },
        }
    }
}
