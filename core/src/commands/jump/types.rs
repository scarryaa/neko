use std::{fmt::Display, str::FromStr};

#[derive(Debug)]
pub enum JumpManagementResult {
    None,
    Aliases(Vec<JumpAliasInfo>),
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum JumpManagementCommand {
    AddAlias { name: String, spec: String },
    RemoveAlias { name: String },
    ListAliases,
}

impl JumpManagementCommand {
    fn parse_add_alias(argument: &str) -> Option<Self> {
        // Normalize to "name:spec"
        let mut normalized_argument = argument.trim();

        // Strip "jump:"
        if let Some(rest) = normalized_argument.strip_prefix("jump:") {
            normalized_argument = rest.trim();
        }

        // Strip "add alias"
        if let Some(rest) = normalized_argument.strip_prefix("add alias") {
            normalized_argument = rest.trim();
        }

        // Should be "name:spec"
        let (name, spec) = normalized_argument.split_once(':')?;

        Some(JumpManagementCommand::AddAlias {
            name: name.trim().to_string(),
            spec: spec.trim().to_string(),
        })
    }

    fn parse_remove_alias(argument: &str) -> Option<Self> {
        let mut normalized_argument = argument.trim();

        // Strip "jump:"
        if let Some(rest) = normalized_argument.strip_prefix("jump:") {
            normalized_argument = rest.trim();
        }

        // Strip "remove alias"
        if let Some(rest) = normalized_argument.strip_prefix("remove alias") {
            normalized_argument = rest.trim();
        }

        if normalized_argument.is_empty() {
            return None;
        }

        // Should be "name"
        Some(JumpManagementCommand::RemoveAlias {
            name: normalized_argument.to_string(),
        })
    }

    pub fn key(&self) -> &'static str {
        match self {
            JumpManagementCommand::AddAlias { .. } => "Jump::AddAlias",
            JumpManagementCommand::RemoveAlias { .. } => "Jump::RemoveAlias",
            JumpManagementCommand::ListAliases => "Jump::ListAliases",
        }
    }

    pub fn encode_argument(&self) -> String {
        match self {
            JumpManagementCommand::AddAlias { name, spec } => {
                format!("{name}:{spec}")
            }
            JumpManagementCommand::RemoveAlias { name } => name.clone(),
            JumpManagementCommand::ListAliases => String::new(),
        }
    }

    pub fn all() -> Vec<Self> {
        vec![
            JumpManagementCommand::ListAliases,
            JumpManagementCommand::AddAlias {
                name: "<name>".into(),
                spec: "<spec>".into(),
            },
            JumpManagementCommand::RemoveAlias {
                name: "<name>".into(),
            },
        ]
    }

    pub fn decode(key: &str, argument: String) -> Option<Self> {
        match key {
            "Jump::AddAlias" => Self::parse_add_alias(&argument),
            "Jump::RemoveAlias" => Self::parse_remove_alias(&argument),
            "Jump::ListAliases" => Some(JumpManagementCommand::ListAliases),
            _ => None,
        }
    }
}

impl Display for JumpManagementCommand {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            JumpManagementCommand::AddAlias { name, spec } => {
                write!(f, "jump: add alias {name}:{spec}")
            }
            JumpManagementCommand::RemoveAlias { name } => {
                write!(f, "jump: remove alias {name}")
            }
            JumpManagementCommand::ListAliases => {
                write!(f, "jump: list aliases")
            }
        }
    }
}

#[derive(Debug, Clone)]
pub struct JumpAliasInfo {
    pub name: String,        // "db"
    pub spec: String,        // "doc.start"
    pub description: String, // "document beginning"
    pub is_builtin: bool,    // "true"
}

#[derive(Default, Clone, Debug)]
pub struct JumpHistory {
    pub previous_jump_command: Option<JumpCommand>,
}

impl JumpHistory {
    pub fn store(&mut self, command: JumpCommand) {
        self.previous_jump_command = Some(command);
    }

    pub fn last(&self) -> Option<&JumpCommand> {
        self.previous_jump_command.as_ref()
    }
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum LineTarget {
    Start,
    Middle,
    End,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum DocumentTarget {
    Start,
    Middle,
    End,
    Quarter,
    ThreeQuarters,
}

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum JumpCommand {
    /// Jump to an explicit position (for "line:col" commands)
    ToPosition { row: usize, column: usize },
    /// Jump within the current line
    ToLine(LineTarget),
    /// Jump within the document
    ToDocument(DocumentTarget),
    /// Jump to the last jump target
    ToLastTarget,
}

impl JumpCommand {
    pub const LAST_JUMP_KEY: &'static str = "ls";

    /// ID used by the command palette / keymaps.
    pub fn key(&self) -> Option<&'static str> {
        match self {
            JumpCommand::ToPosition { .. } => Some("goto"),

            JumpCommand::ToLine(LineTarget::Start) => Some("lb"),
            JumpCommand::ToLine(LineTarget::Middle) => Some("lm"),
            JumpCommand::ToLine(LineTarget::End) => Some("le"),

            JumpCommand::ToDocument(DocumentTarget::Start) => Some("db"),
            JumpCommand::ToDocument(DocumentTarget::Middle) => Some("dm"),
            JumpCommand::ToDocument(DocumentTarget::End) => Some("de"),
            JumpCommand::ToDocument(DocumentTarget::Quarter) => Some("dh"),
            JumpCommand::ToDocument(DocumentTarget::ThreeQuarters) => Some("dt"),

            JumpCommand::ToLastTarget => Some(Self::LAST_JUMP_KEY),
        }
    }

    /// All jump commands.
    pub fn all() -> Vec<JumpCommand> {
        vec![
            JumpCommand::ToLine(LineTarget::Start),
            JumpCommand::ToLine(LineTarget::Middle),
            JumpCommand::ToLine(LineTarget::End),
            JumpCommand::ToDocument(DocumentTarget::Start),
            JumpCommand::ToDocument(DocumentTarget::Middle),
            JumpCommand::ToDocument(DocumentTarget::End),
            JumpCommand::ToDocument(DocumentTarget::Quarter),
            JumpCommand::ToDocument(DocumentTarget::ThreeQuarters),
            JumpCommand::ToLastTarget,
        ]
    }

    pub fn from_spec(spec: &str) -> Option<Self> {
        match spec {
            "line.start" => Some(JumpCommand::ToLine(LineTarget::Start)),
            "line.middle" => Some(JumpCommand::ToLine(LineTarget::Middle)),
            "line.end" => Some(JumpCommand::ToLine(LineTarget::End)),

            "doc.start" => Some(JumpCommand::ToDocument(DocumentTarget::Start)),
            "doc.middle" => Some(JumpCommand::ToDocument(DocumentTarget::Middle)),
            "doc.end" => Some(JumpCommand::ToDocument(DocumentTarget::End)),
            "doc.quarter" => Some(JumpCommand::ToDocument(DocumentTarget::Quarter)),
            "doc.three_quarters" => Some(JumpCommand::ToDocument(DocumentTarget::ThreeQuarters)),

            _ => None,
        }
    }
}

impl FromStr for JumpCommand {
    type Err = &'static str;

    fn from_str(s: &str) -> Result<Self, Self::Err> {
        match s {
            "lb" => Ok(JumpCommand::ToLine(LineTarget::Start)),
            "lm" => Ok(JumpCommand::ToLine(LineTarget::Middle)),
            "le" => Ok(JumpCommand::ToLine(LineTarget::End)),

            "db" => Ok(JumpCommand::ToDocument(DocumentTarget::Start)),
            "dm" => Ok(JumpCommand::ToDocument(DocumentTarget::Middle)),
            "de" => Ok(JumpCommand::ToDocument(DocumentTarget::End)),
            "dh" => Ok(JumpCommand::ToDocument(DocumentTarget::Quarter)),
            "dt" => Ok(JumpCommand::ToDocument(DocumentTarget::ThreeQuarters)),

            Self::LAST_JUMP_KEY => Ok(JumpCommand::ToLastTarget),

            _ => Err("Unknown jump command"),
        }
    }
}

impl Display for JumpCommand {
    fn fmt(&self, f: &mut std::fmt::Formatter<'_>) -> std::fmt::Result {
        match self {
            JumpCommand::ToPosition { row, column } => {
                write!(f, "go to {}:{}", row + 1, column + 1)
            }
            JumpCommand::ToLine(LineTarget::Start) => write!(f, "line beginning"),
            JumpCommand::ToLine(LineTarget::Middle) => write!(f, "line middle"),
            JumpCommand::ToLine(LineTarget::End) => write!(f, "line end"),

            JumpCommand::ToDocument(DocumentTarget::Start) => write!(f, "document beginning"),
            JumpCommand::ToDocument(DocumentTarget::Middle) => write!(f, "document middle"),
            JumpCommand::ToDocument(DocumentTarget::End) => write!(f, "document end"),
            JumpCommand::ToDocument(DocumentTarget::Quarter) => write!(f, "document 1/4"),
            JumpCommand::ToDocument(DocumentTarget::ThreeQuarters) => {
                write!(f, "document 3/4")
            }

            JumpCommand::ToLastTarget => write!(f, "repeat last jump"),
        }
    }
}
