use crate::TabId;
use std::fmt;

pub type TabResult<T> = std::result::Result<T, TabError>;

#[derive(Debug)]
pub enum MoveTabError {
    InvalidIndex { from: usize, to: usize },
}

impl fmt::Display for MoveTabError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            MoveTabError::InvalidIndex { from, to } => {
                write!(f, "Tab index out of range (from: {from}, to: {to})")
            }
        }
    }
}

impl From<MoveTabError> for TabError {
    fn from(err: MoveTabError) -> Self {
        TabError::Move(err)
    }
}

#[derive(Debug)]
pub enum TabError {
    InvalidId(u64),
    NoIdProvided,
    NotFound(TabId),
    Move(MoveTabError),
}

impl fmt::Display for TabError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            TabError::InvalidId(_) => write!(f, "Tab id must not be 0"),
            TabError::NoIdProvided => write!(f, "No tab id was provided"),
            TabError::NotFound(id) => write!(f, "Tab {id} not found"),
            TabError::Move(err) => write!(f, "{err}"),
        }
    }
}
