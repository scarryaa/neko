pub mod tab;

use std::{fmt::Display, str::FromStr};

use crate::{AppState, config::ConfigManager, theme::ThemeManager};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Command {
    FileExplorerToggle,
    ChangeTheme(String),
    OpenConfig,
}

impl Command {
    pub fn key(&self) -> String {
        match self {
            Command::FileExplorerToggle => "FileExplorer::Toggle".into(),
            Command::ChangeTheme(name) => format!("Theme::{name}"),
            Command::OpenConfig => "Editor::OpenConfig".into(),
        }
    }
}

impl FromStr for Command {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        if s == "FileExplorer::Toggle" {
            return Ok(Command::FileExplorerToggle);
        }
        if let Some(rest) = s.strip_prefix("Theme::") {
            return Ok(Command::ChangeTheme(rest.to_string()));
        }
        Err("Unknown command")
    }
}

impl Display for Command {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            Command::FileExplorerToggle => write!(f, "Toggle file explorer"),
            Command::ChangeTheme(name) => write!(f, "Change theme to {name}"),
            Command::OpenConfig => write!(f, "Open config in editor"),
        }
    }
}

#[derive(Debug, Clone)]
pub enum UiIntent {
    ToggleFileExplorer,
    ApplyTheme { name: String },
    OpenConfig { id: i64, path: String },
}

pub struct CommandResult {
    pub intents: Vec<UiIntent>,
}

pub fn execute_command(
    cmd: Command,
    config: &mut ConfigManager,
    theme: &mut ThemeManager,
    app: &mut AppState,
) -> CommandResult {
    match cmd {
        Command::FileExplorerToggle => {
            let new_state = !config.get_snapshot().file_explorer_shown;
            config.update(|c| c.file_explorer_shown = new_state);

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
