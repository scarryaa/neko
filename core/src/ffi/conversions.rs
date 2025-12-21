use super::bridge::ffi::*;
use crate::{
    Command, CommandResult, Cursor, UiIntent,
    text::{ChangeSet, cursor_manager::AddCursorDirection},
};

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
