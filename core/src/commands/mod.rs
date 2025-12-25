pub mod tab;

use std::{fmt::Display, str::FromStr};

use crate::{config::ConfigManager, theme::ThemeManager};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Command {
    FileExplorerToggle,
    ChangeTheme(String),
}

impl Command {
    pub fn key(&self) -> String {
        match self {
            Command::FileExplorerToggle => "FileExplorer::Toggle".into(),
            Command::ChangeTheme(name) => format!("Theme::{name}"),
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
        }
    }
}

#[derive(Debug, Clone)]
pub enum UiIntent {
    ToggleFileExplorer,
    ApplyTheme { name: String },
}

pub struct CommandResult {
    pub intents: Vec<UiIntent>,
}

pub fn execute_command(
    cmd: Command,
    config: &mut ConfigManager,
    theme: &mut ThemeManager,
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
    }
}
