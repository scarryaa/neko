use super::{Command, CommandResult, UiIntent};
use crate::{AppState, ConfigManager, ThemeManager};

pub fn execute_command(
    cmd: Command,
    config: &mut ConfigManager,
    theme: &mut ThemeManager,
    app: &mut AppState,
) -> CommandResult {
    match cmd {
        Command::FileExplorerToggle => {
            let new_state = !config.get_snapshot().file_explorer.shown;
            config.update(|c| c.file_explorer.shown = new_state);

            CommandResult {
                intents: vec![UiIntent::ToggleFileExplorer],
            }
        }
        Command::ChangeTheme(name) => {
            theme.set_theme(&name);
            config.update(|c| c.current_theme = name.clone());

            CommandResult {
                intents: vec![UiIntent::ApplyTheme { name }],
            }
        }
        Command::OpenConfig => {
            // TODO(scarlet): Improve the flow here, i.e. make new_tab + open_file atomic
            let config_path = ConfigManager::get_config_path();
            let config_path_str = config_path.to_string_lossy().to_string();

            // Try to find an existing tab
            let existing_id = app.get_tabs().iter().find_map(|t| {
                t.get_file_path().and_then(|p| {
                    if p.to_string_lossy() == config_path_str {
                        Some(t.get_id())
                    } else {
                        None
                    }
                })
            });

            let id = if let Some(id) = existing_id {
                id
            } else {
                app.new_tab(true);
                app.open_file(&config_path_str).unwrap_or_default()
            };

            CommandResult {
                intents: vec![UiIntent::OpenConfig {
                    id: id as i64,
                    path: config_path_str,
                }],
            }
        }
    }
}

pub fn get_available_commands() -> std::io::Result<Vec<Command>> {
    Ok(Command::all().to_vec())
}
