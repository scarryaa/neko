use std::{fmt::Display, str::FromStr};

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
