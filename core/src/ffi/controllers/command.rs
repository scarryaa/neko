use crate::{
    AppState, ConfigManager, FileExplorerCommand, TabCommand, ThemeManager,
    commands::{FileExplorerCommandError, TabCommandError},
    execute_command, execute_jump_command, execute_jump_key,
    ffi::{
        CommandFfi, CommandKindFfi, CommandResultFfi, FileExplorerCommandFfi,
        FileExplorerCommandKindFfi, FileExplorerCommandResultFfi, FileExplorerCommandStateFfi,
        FileExplorerContextFfi, JumpCommandFfi, TabCommandFfi, TabCommandKindFfi,
        TabCommandStateFfi, TabContextFfi,
    },
    file_explorer_command_state, get_available_commands, get_available_file_explorer_commands,
    get_available_jump_commands, get_available_tab_commands, run_file_explorer_command,
    run_tab_command, tab_command_state,
};
use std::{cell::RefCell, rc::Rc};

pub struct CommandController {
    pub(crate) app_state: Rc<RefCell<AppState>>,
}

impl CommandController {
    // Jump Commands
    pub fn execute_jump_command(&self, cmd: JumpCommandFfi) {
        let mut app_state = self.app_state.borrow_mut();
        let Ok(command) = cmd.clone().try_into() else {
            eprintln!("Invalid JumpCommandFfi from C++: {cmd:?}");
            return;
        };

        execute_jump_command(command, &mut app_state);
    }

    pub fn execute_jump_key(&self, key: String) {
        let mut app_state = self.app_state.borrow_mut();
        execute_jump_key(key, &mut app_state);
    }

    pub fn get_available_jump_commands(&self) -> Vec<JumpCommandFfi> {
        let Ok(commands) = get_available_jump_commands() else {
            return Vec::new();
        };

        commands.into_iter().map(|cmd| cmd.into()).collect()
    }

    // File Explorer Commands
    pub fn run_file_explorer_command(
        &self,
        id: &str,
        ctx: FileExplorerContextFfi,
        new_or_rename_item_name: String,
    ) -> FileExplorerCommandResultFfi {
        let mut app_state = self.app_state.borrow_mut();
        let command_result = run_file_explorer_command(
            &mut app_state,
            id,
            &ctx.into(),
            Some(new_or_rename_item_name),
        );

        if let Ok(result) = command_result {
            result.into()
        } else {
            eprintln!(
                "File Explorer Command failed: {}",
                command_result.unwrap_err()
            );

            FileExplorerCommandResultFfi {
                intents: Vec::new(),
            }
        }
    }

    pub fn parse_file_explorer_command_id(
        &self,
        id: &str,
    ) -> Result<FileExplorerCommandKindFfi, FileExplorerCommandError> {
        id.parse::<FileExplorerCommand>()
            .map(Into::into)
            .map_err(|_| FileExplorerCommandError::ParseError(id.to_string()))
    }

    pub fn get_file_explorer_command_state(&self, path: String) -> FileExplorerCommandStateFfi {
        let app_state = self.app_state.borrow();

        match file_explorer_command_state(&app_state, path.into()) {
            Ok(state) => state.into(),
            Err(_) => FileExplorerCommandStateFfi::default(),
        }
    }

    pub fn get_available_file_explorer_commands(
        &self,
        ctx: FileExplorerContextFfi,
    ) -> Vec<FileExplorerCommandFfi> {
        let Ok(commands) = get_available_file_explorer_commands(ctx.into()) else {
            return Vec::new();
        };

        commands
            .into_iter()
            .map(|cmd| FileExplorerCommandFfi {
                id: cmd.as_str().to_string(),
            })
            .collect()
    }

    // Tab Commands
    pub fn run_tab_command(&self, id: &str, ctx: TabContextFfi, close_pinned: bool) -> bool {
        let mut app_state = self.app_state.borrow_mut();
        run_tab_command(&mut app_state, id, &ctx.into(), close_pinned).is_ok()
    }

    pub fn get_tab_command_state(&self, id: u64) -> TabCommandStateFfi {
        let app_state = self.app_state.borrow();

        match tab_command_state(&app_state, id.into()) {
            Ok(state) => state.into(),
            Err(_) => TabCommandStateFfi {
                can_close: false,
                can_close_others: false,
                can_close_left: false,
                can_close_right: false,
                can_close_all: false,
                can_close_clean: false,
                can_copy_path: false,
                can_reveal: false,
                is_pinned: false,
            },
        }
    }

    pub fn get_available_tab_commands(&self) -> Vec<TabCommandFfi> {
        let Ok(commands) = get_available_tab_commands() else {
            return Vec::new();
        };

        commands
            .into_iter()
            .map(|cmd| TabCommandFfi {
                id: cmd.as_str().to_string(),
            })
            .collect()
    }

    pub fn parse_tab_command_id(
        self: &CommandController,
        id: &str,
    ) -> Result<TabCommandKindFfi, TabCommandError> {
        id.parse::<TabCommand>()
            .map(Into::into)
            .map_err(|_| TabCommandError::ParseError(id.to_string()))
    }

    // General Commands
    pub(crate) fn new_command(
        &self,
        key: String,
        display_name: String,
        kind: CommandKindFfi,
        argument: String,
    ) -> CommandFfi {
        CommandFfi {
            key,
            display_name,
            kind,
            argument,
        }
    }

    pub fn execute_command(
        &self,
        cmd: CommandFfi,
        config: &mut ConfigManager,
        theme: &mut ThemeManager,
    ) -> CommandResultFfi {
        let mut app_state = self.app_state.borrow_mut();
        if let Ok(result) = execute_command(cmd.into(), config, theme, &mut app_state) {
            result.into()
        } else {
            CommandResultFfi {
                intents: Vec::new(),
            }
        }
    }

    pub fn get_available_commands(&self) -> Vec<CommandFfi> {
        let Ok(commands) = get_available_commands() else {
            return Vec::new();
        };

        commands.into_iter().map(|cmd| cmd.into()).collect()
    }
}
