use super::*;
use crate::{
    AddCursorDirection, ChangeSet, CloseTabOperationType, Command, CommandResult, Config, Cursor,
    DocumentError, DocumentTarget, FileSystemError, JumpAliasInfo, JumpCommand,
    JumpManagementCommand, LineTarget, OpenTabResult, TabCommandState, TabContext, UiIntent,
};
use std::{fmt, io, path::PathBuf};

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

impl From<OpenTabResult> for OpenTabResultFfi {
    fn from(result: OpenTabResult) -> Self {
        OpenTabResultFfi {
            tab_id: result.tab_id.map(|id| id.into()).unwrap_or(0),
            found_tab_id: result.tab_id.is_some(),
            tab_already_exists: result.tab_already_exists,
        }
    }
}

impl From<CloseTabOperationType> for CloseTabOperationTypeFfi {
    fn from(operation_type: CloseTabOperationType) -> Self {
        match operation_type {
            CloseTabOperationType::Single => CloseTabOperationTypeFfi::Single,
            CloseTabOperationType::Left => CloseTabOperationTypeFfi::Left,
            CloseTabOperationType::Right => CloseTabOperationTypeFfi::Right,
            CloseTabOperationType::Others => CloseTabOperationTypeFfi::Others,
            CloseTabOperationType::Clean => CloseTabOperationTypeFfi::Clean,
            CloseTabOperationType::All => CloseTabOperationTypeFfi::All,
        }
    }
}

impl From<CloseTabOperationTypeFfi> for CloseTabOperationType {
    fn from(operation_type: CloseTabOperationTypeFfi) -> Self {
        match operation_type {
            CloseTabOperationTypeFfi::Single => Self::Single,
            CloseTabOperationTypeFfi::Left => Self::Left,
            CloseTabOperationTypeFfi::Right => Self::Right,
            CloseTabOperationTypeFfi::Others => Self::Others,
            CloseTabOperationTypeFfi::Clean => Self::Clean,
            CloseTabOperationTypeFfi::All => Self::All,
            _ => unreachable!(
                "Conversion from CloseTabOperationTypeFfi to CloseTabOperationType failed"
            ),
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
            UiIntentKindFfi::ShowJumpAliases => UiIntent::ShowJumpAliases {
                aliases: intent
                    .jump_aliases
                    .into_iter()
                    .map(|alias| alias.into())
                    .collect(),
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
                jump_aliases: Vec::new(),
            },
            UiIntent::ApplyTheme { name } => UiIntentFfi {
                kind: UiIntentKindFfi::ApplyTheme,
                argument_str: name,
                argument_u64: 0,
                jump_aliases: Vec::new(),
            },
            UiIntent::OpenConfig { id, path } => UiIntentFfi {
                kind: UiIntentKindFfi::OpenConfig,
                argument_str: path,
                argument_u64: id.into(),
                jump_aliases: Vec::new(),
            },
            UiIntent::ShowJumpAliases { aliases } => {
                let aliases_ffi = aliases
                    .iter()
                    .map(|alias| JumpAliasInfoFfi::from(alias.clone()))
                    .collect();

                UiIntentFfi {
                    kind: UiIntentKindFfi::ShowJumpAliases,
                    argument_str: String::new(),
                    argument_u64: 0,
                    jump_aliases: aliases_ffi,
                }
            }
        }
    }
}

impl From<CommandFfi> for Command {
    fn from(command: CommandFfi) -> Self {
        match command.kind {
            CommandKindFfi::FileExplorerToggle => Command::FileExplorerToggle,
            CommandKindFfi::ChangeTheme => Command::ChangeTheme(command.argument),
            CommandKindFfi::OpenConfig => Command::OpenConfig,
            CommandKindFfi::JumpManagement => {
                // Try to decode
                if let Some(command) =
                    JumpManagementCommand::decode(&command.key, command.argument.clone())
                {
                    Command::JumpManagement(command)
                } else {
                    eprintln!(
                        "Failed to decode JumpManagementCommand from key='{}', arg='{}'",
                        command.key, command.argument
                    );
                    Command::NoOp
                }
            }
            _ => {
                eprintln!("Conversion from CommandFfi to Command failed: defaulting to NoOp");
                Command::NoOp
            }
        }
    }
}

impl From<Command> for CommandFfi {
    fn from(command: Command) -> Self {
        match command {
            Command::FileExplorerToggle => CommandFfi {
                key: command.key(),
                display_name: command.to_string(),
                kind: CommandKindFfi::FileExplorerToggle,
                argument: String::new(),
            },
            Command::ChangeTheme(ref t) => CommandFfi {
                key: command.key(),
                display_name: command.to_string(),
                kind: CommandKindFfi::ChangeTheme,
                argument: t.to_string(),
            },
            Command::OpenConfig => CommandFfi {
                key: command.key(),
                display_name: command.to_string(),
                kind: CommandKindFfi::OpenConfig,
                argument: String::new(),
            },
            Command::JumpManagement(ref command) => CommandFfi {
                key: command.key().to_string(),
                display_name: command.to_string(),
                kind: CommandKindFfi::JumpManagement,
                argument: command.encode_argument(),
            },
            Command::NoOp => CommandFfi {
                key: "".to_string(),
                display_name: "".to_string(),
                kind: CommandKindFfi::NoOp,
                argument: "".to_string(),
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

// Jump Conversions
impl TryFrom<LineTargetFfi> for LineTarget {
    type Error = &'static str;

    fn try_from(value: LineTargetFfi) -> Result<Self, Self::Error> {
        Ok(match value {
            LineTargetFfi::Start => LineTarget::Start,
            LineTargetFfi::Middle => LineTarget::Middle,
            LineTargetFfi::End => LineTarget::End,
            _ => return Err("Unknown LineTargetFfi value"),
        })
    }
}

impl TryFrom<DocumentTargetFfi> for DocumentTarget {
    type Error = &'static str;

    fn try_from(value: DocumentTargetFfi) -> Result<Self, Self::Error> {
        Ok(match value {
            DocumentTargetFfi::Start => DocumentTarget::Start,
            DocumentTargetFfi::Middle => DocumentTarget::Middle,
            DocumentTargetFfi::End => DocumentTarget::End,
            DocumentTargetFfi::Quarter => DocumentTarget::Quarter,
            DocumentTargetFfi::ThreeQuarters => DocumentTarget::ThreeQuarters,
            _ => return Err("Unknown DocumentTargetFfi value"),
        })
    }
}

impl TryFrom<JumpCommandFfi> for JumpCommand {
    type Error = &'static str;

    fn try_from(cmd: JumpCommandFfi) -> Result<Self, Self::Error> {
        Ok(match cmd.kind {
            JumpCommandKindFfi::ToPosition => JumpCommand::ToPosition {
                row: usize::try_from(cmd.row).unwrap_or(usize::MAX),
                column: usize::try_from(cmd.column).unwrap_or(usize::MAX),
            },
            JumpCommandKindFfi::ToLine => {
                JumpCommand::ToLine(LineTarget::try_from(cmd.line_target)?)
            }
            JumpCommandKindFfi::ToDocument => {
                JumpCommand::ToDocument(DocumentTarget::try_from(cmd.document_target)?)
            }
            JumpCommandKindFfi::ToLastTarget => JumpCommand::ToLastTarget,
            _ => return Err("Unknown jump command kind"),
        })
    }
}

impl From<LineTarget> for LineTargetFfi {
    fn from(target: LineTarget) -> Self {
        match target {
            LineTarget::Start => LineTargetFfi::Start,
            LineTarget::Middle => LineTargetFfi::Middle,
            LineTarget::End => LineTargetFfi::End,
        }
    }
}

impl From<DocumentTarget> for DocumentTargetFfi {
    fn from(target: DocumentTarget) -> Self {
        match target {
            DocumentTarget::Start => DocumentTargetFfi::Start,
            DocumentTarget::Middle => DocumentTargetFfi::Middle,
            DocumentTarget::End => DocumentTargetFfi::End,
            DocumentTarget::Quarter => DocumentTargetFfi::Quarter,
            DocumentTarget::ThreeQuarters => DocumentTargetFfi::ThreeQuarters,
        }
    }
}

impl From<JumpCommand> for JumpCommandFfi {
    fn from(command: JumpCommand) -> Self {
        match command {
            JumpCommand::ToLine(ref target) => JumpCommandFfi {
                key: command.key().unwrap().to_string(),
                display_name: command.to_string(),
                kind: JumpCommandKindFfi::ToLine,
                row: 0,
                column: 0,
                line_target: LineTargetFfi::from(target.clone()),
                document_target: DocumentTargetFfi::Start,
            },

            JumpCommand::ToDocument(ref target) => JumpCommandFfi {
                key: command.key().unwrap().to_string(),
                display_name: command.to_string(),
                kind: JumpCommandKindFfi::ToDocument,
                row: 0,
                column: 0,
                line_target: LineTargetFfi::Start,
                document_target: DocumentTargetFfi::from(target.clone()),
            },

            JumpCommand::ToPosition { row, column } => JumpCommandFfi {
                key: command.key().unwrap().to_string(),
                display_name: command.to_string(),
                kind: JumpCommandKindFfi::ToPosition,
                row: row as i64,
                column: column as i64,
                line_target: LineTargetFfi::Start,
                document_target: DocumentTargetFfi::Start,
            },

            JumpCommand::ToLastTarget => JumpCommandFfi {
                key: command.key().unwrap().to_string(),
                display_name: command.to_string(),
                kind: JumpCommandKindFfi::ToLastTarget,
                row: 0,
                column: 0,
                line_target: LineTargetFfi::Start,
                document_target: DocumentTargetFfi::Start,
            },
        }
    }
}

impl From<FileSystemError> for FileSystemErrorFfi {
    fn from(error: FileSystemError) -> Self {
        match error {
            FileSystemError::Io(_) => FileSystemErrorFfi::Io,
            FileSystemError::MissingName => FileSystemErrorFfi::MissingName,
            FileSystemError::BadSystemTime(_) => FileSystemErrorFfi::BadSystemTime,
        }
    }
}

impl fmt::Display for FileSystemErrorFfi {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            FileSystemErrorFfi::MissingName => write!(f, "Missing name"),
            FileSystemErrorFfi::BadSystemTime => write!(f, "Bad system time"),
            FileSystemErrorFfi::Io => write!(f, "IO error"),
            _ => unreachable!("FileSystemErrorFfi Display cases should be handled"),
        }
    }
}

impl fmt::Display for DocumentErrorFfi {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match *self {
            DocumentErrorFfi::Io => write!(f, "IO error"),
            DocumentErrorFfi::InvalidId => write!(f, "Document id must not be 0"),
            DocumentErrorFfi::NoPath => write!(f, "Document has no path"),
            DocumentErrorFfi::NotFound => write!(f, "Document not found"),
            _ => unreachable!("DocumentErrorFfi Display cases should be handled"),
        }
    }
}

impl From<io::Error> for DocumentErrorFfi {
    fn from(_: io::Error) -> Self {
        DocumentErrorFfi::Io
    }
}

impl From<DocumentError> for DocumentErrorFfi {
    fn from(error: DocumentError) -> Self {
        match error {
            DocumentError::Io(_) => DocumentErrorFfi::Io,
            DocumentError::InvalidId(_) => DocumentErrorFfi::InvalidId,
            DocumentError::NoPath(_) => DocumentErrorFfi::NoPath,
            DocumentError::NotFound(_) => DocumentErrorFfi::NotFound,
            DocumentError::ViewNotFound => DocumentErrorFfi::ViewNotFound,
        }
    }
}

// Jump Alias Command Conversions
impl From<JumpAliasInfo> for JumpAliasInfoFfi {
    fn from(alias_info: JumpAliasInfo) -> Self {
        Self {
            name: alias_info.name,
            spec: alias_info.spec,
            description: alias_info.description,
            is_builtin: alias_info.is_builtin,
        }
    }
}

impl From<JumpAliasInfoFfi> for JumpAliasInfo {
    fn from(alias_info: JumpAliasInfoFfi) -> Self {
        Self {
            name: alias_info.name,
            spec: alias_info.spec,
            description: alias_info.description,
            is_builtin: alias_info.is_builtin,
        }
    }
}

// Tab Command Conversions
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
            id: ctx.id.into(),
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
