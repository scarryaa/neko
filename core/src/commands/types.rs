use crate::{JumpAliasInfo, JumpManagementCommand};
use std::{fmt::Display, str::FromStr};

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum Command {
    FileExplorerToggle,
    ChangeTheme(String),
    OpenConfig,
    JumpManagement(JumpManagementCommand),
    /// Used internally if no match was found or conversion fails
    NoOp,
}

impl Command {
    pub fn key(&self) -> String {
        match self {
            Command::FileExplorerToggle => "FileExplorer::Toggle".into(),
            Command::ChangeTheme(name) => format!("Theme::{name}"),
            Command::OpenConfig => "Editor::OpenConfig".into(),
            Command::JumpManagement(command) => command.key().into(),
            Command::NoOp => "".to_string(),
        }
    }

    pub fn all() -> Vec<Command> {
        let mut cmds = vec![
            Command::FileExplorerToggle,
            Command::ChangeTheme("Default Dark".to_string()),
            Command::ChangeTheme("Default Light".to_string()),
            Command::OpenConfig,
        ];

        cmds.extend(
            JumpManagementCommand::all()
                .into_iter()
                .map(Command::JumpManagement),
        );

        cmds
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
            Command::FileExplorerToggle => write!(f, "file explorer: toggle"),
            Command::ChangeTheme(name) => write!(f, "theme: {}", name.to_lowercase()),
            Command::OpenConfig => write!(f, "editor: open config"),
            Command::JumpManagement(command) => write!(f, "{command}"),
            Command::NoOp => write!(f, ""),
        }
    }
}

#[derive(Debug, Clone)]
pub enum UiIntent {
    ToggleFileExplorer,
    ApplyTheme { name: String },
    OpenConfig { id: i64, path: String },
    ShowJumpAliases { aliases: Vec<JumpAliasInfo> },
}

pub struct CommandResult {
    pub intents: Vec<UiIntent>,
}
