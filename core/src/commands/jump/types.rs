use std::{fmt::Display, str::FromStr};

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
            JumpCommand::ToLine(LineTarget::Start) => write!(f, "go to line: start"),
            JumpCommand::ToLine(LineTarget::Middle) => write!(f, "go to line: middle"),
            JumpCommand::ToLine(LineTarget::End) => write!(f, "go to line: end"),

            JumpCommand::ToDocument(DocumentTarget::Start) => write!(f, "go to document: start"),
            JumpCommand::ToDocument(DocumentTarget::Middle) => write!(f, "go to document: middle"),
            JumpCommand::ToDocument(DocumentTarget::End) => write!(f, "go to document: end"),
            JumpCommand::ToDocument(DocumentTarget::Quarter) => write!(f, "go to document: 1/4"),
            JumpCommand::ToDocument(DocumentTarget::ThreeQuarters) => {
                write!(f, "go to document: 3/4")
            }

            JumpCommand::ToLastTarget => write!(f, "repeat last jump"),
        }
    }
}
