use std::path::PathBuf;

use super::bridge::ffi::*;
use crate::{
    Command, CommandResult, Config, Cursor, UiIntent,
    commands::tab::{TabCommandState, TabContext},
    text::{ChangeSet, cursor_manager::AddCursorDirection},
};

impl From<Config> for ConfigSnapshotFfi {
    fn from(c: Config) -> Self {
        let (dir_present, dir) = match c.file_explorer.directory {
            Some(d) => (true, d),
            None => (false, String::new()),
        };

        Self {
            editor: EditorConfigFfi {
                font_size: c.editor.font_size as u32,
                font_family: c.editor.font_family,
            },
            file_explorer: FileExplorerConfigFfi {
                font_size: c.file_explorer.font_size as u32,
                font_family: c.file_explorer.font_family,
                directory_present: dir_present,
                directory: dir,
                shown: c.file_explorer.shown,
                width: c.file_explorer.width as u32,
                right: c.file_explorer.right,
            },
            interface: InterfaceConfigFfi {
                font_size: c.interface.font_size as u32,
                font_family: c.interface.font_family,
            },
            terminal: TerminalConfigFfi {
                font_size: c.terminal.font_size as u32,
                font_family: c.terminal.font_family,
            },
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
                name: intent.argument_str,
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
                argument_str: String::new(),
                argument_u64: 0,
            },
            UiIntent::ApplyTheme { name } => UiIntentFfi {
                kind: UiIntentKindFfi::ApplyTheme,
                argument_str: name,
                argument_u64: 0,
            },
            UiIntent::OpenConfig { id, path } => UiIntentFfi {
                kind: UiIntentKindFfi::OpenConfig,
                argument_str: path,
                argument_u64: id as u64,
            },
        }
    }
}

impl From<CommandFfi> for Command {
    fn from(command: CommandFfi) -> Self {
        match command.kind {
            CommandKindFfi::FileExplorerToggle => Command::FileExplorerToggle,
            CommandKindFfi::ChangeTheme => Command::ChangeTheme(command.argument),
            CommandKindFfi::OpenConfig => Command::OpenConfig,
            // Should not happen
            _ => {
                eprintln!(
                    "Conversion from CommandFfi to Command failed: defaulting to FileExplorerToggle"
                );
                Command::FileExplorerToggle
            }
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
            Command::OpenConfig => CommandFfi {
                kind: CommandKindFfi::OpenConfig,
                argument: String::new(),
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
