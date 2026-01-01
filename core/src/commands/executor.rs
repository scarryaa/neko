use crate::{
    AppState, Command, CommandResult, ConfigManager, JumpManagementResult, ThemeManager, UiIntent,
    execute_jump_management_command,
};
use std::io::{Error, ErrorKind};

pub fn execute_command(
    cmd: Command,
    config: &mut ConfigManager,
    theme: &mut ThemeManager,
    app: &mut AppState,
) -> Result<CommandResult, std::io::Error> {
    match cmd {
        Command::FileExplorerToggle => {
            let new_state = !config.get_snapshot().file_explorer.shown;
            config.update(|c| c.file_explorer.shown = new_state);

            Ok(CommandResult {
                intents: vec![UiIntent::ToggleFileExplorer],
            })
        }
        Command::ChangeTheme(name) => {
            theme.set_theme(&name);
            config.update(|c| c.current_theme = name.clone());

            Ok(CommandResult {
                intents: vec![UiIntent::ApplyTheme { name }],
            })
        }
        Command::OpenConfig => {
            let config_path = ConfigManager::get_config_path();

            let tab_id = app
                .ensure_tab_for_path(&config_path, true)
                .map_err(|err| Error::new(ErrorKind::NotFound, err))?;

            let config_path_str = config_path.to_string_lossy().to_string();

            Ok(CommandResult {
                intents: vec![UiIntent::OpenConfig {
                    id: tab_id,
                    path: config_path_str,
                }],
            })
        }
        Command::JumpManagement(cmd) => {
            let result = execute_jump_management_command(cmd, config)?;

            // TODO(scarlet): Show a toast for add/remove aliases?
            let intents = match result {
                JumpManagementResult::None => Vec::new(),
                JumpManagementResult::Aliases(aliases) => {
                    vec![UiIntent::ShowJumpAliases { aliases }]
                }
            };

            Ok(CommandResult { intents })
        }
        Command::NoOp => Ok(CommandResult {
            intents: Vec::new(),
        }),
    }
}

pub fn get_available_commands() -> std::io::Result<Vec<Command>> {
    Ok(Command::all().to_vec())
}
