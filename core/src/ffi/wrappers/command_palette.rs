use crate::{
    AppState, ConfigManager, ThemeManager, execute_command, execute_jump_command,
    ffi::{CommandFfi, CommandKindFfi, CommandResultFfi, JumpCommandFfi},
    get_available_commands, get_available_jump_commands,
};

// Command palette commands
pub(crate) fn execute_command_wrapper(
    cmd: CommandFfi,
    config: &mut ConfigManager,
    theme: &mut ThemeManager,
    app: &mut AppState,
) -> CommandResultFfi {
    if let Ok(result) = execute_command(cmd.into(), config, theme, app) {
        result.into()
    } else {
        CommandResultFfi {
            intents: Vec::new(),
        }
    }
}

pub fn get_available_commands_wrapper() -> Vec<CommandFfi> {
    let Ok(commands) = get_available_commands() else {
        return Vec::new();
    };

    commands.into_iter().map(|cmd| cmd.into()).collect()
}

pub(crate) fn new_command(
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

// Jump commands
pub(crate) fn execute_jump_command_wrapper(cmd: JumpCommandFfi, app_state: &mut AppState) {
    let Ok(command) = cmd.clone().try_into() else {
        eprintln!("Invalid JumpCommandFfi from C++: {cmd:?}");
        return;
    };

    execute_jump_command(command, app_state);
}

pub(crate) fn get_available_jump_commands_wrapper() -> Vec<JumpCommandFfi> {
    let Ok(commands) = get_available_jump_commands() else {
        return Vec::new();
    };

    commands.into_iter().map(|cmd| cmd.into()).collect()
}
