use crate::{CursorEntry, Selection};
use std::{fmt, num::NonZeroU64, path::PathBuf};

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

impl std::error::Error for MoveTabError {}

impl From<MoveTabError> for TabError {
    fn from(err: MoveTabError) -> Self {
        TabError::Move(err)
    }
}

#[derive(Debug)]
pub enum TabError {
    InvalidId(u64),
    NotFound(TabId),
    Move(MoveTabError),
}

impl fmt::Display for TabError {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        match self {
            TabError::InvalidId(id) => write!(f, "Tab id {id} must not be 0"),
            TabError::NotFound(id) => write!(f, "Tab {id} not found"),
            TabError::Move(err) => write!(f, "{err}"),
        }
    }
}

impl std::error::Error for TabError {}

/// Represents the id associated with a given [`super::Tab`].
#[derive(Eq, Hash, PartialEq, Clone, Copy, Debug)]
pub struct TabId(NonZeroU64);

impl TabId {
    pub fn new(id: u64) -> Result<Self, TabError> {
        NonZeroU64::new(id).map(Self).ok_or(TabError::InvalidId(id))
    }

    pub fn take_next(&mut self) -> Self {
        let current_id = *self;
        self.0 = self.0.saturating_add(1);
        current_id
    }
}

impl From<TabId> for u64 {
    fn from(id: TabId) -> Self {
        id.0.get()
    }
}

impl fmt::Display for TabId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

#[derive(Debug, PartialEq, Eq)]
pub enum CloseTabOperationType {
    Single,
    Left,
    Right,
    Others,
    Clean,
    All,
}

#[derive(Debug)]
pub struct ClosedTabInfo {
    pub path: PathBuf,
    pub scroll_offsets: (usize, usize),
    pub cursors: Vec<CursorEntry>,
    pub selections: Selection,
}

#[derive(Debug)]
pub struct ScrollOffsets {
    pub x: usize,
    pub y: usize,
}

#[derive(Debug)]
pub struct MoveActiveTabResult {
    pub found_id: Option<TabId>,
    pub reopened_tab: bool,
    pub scroll_offsets: Option<ScrollOffsets>,
    pub cursors: Option<Vec<CursorEntry>>,
    pub selections: Option<Selection>,
    pub reopen_path: Option<PathBuf>,
}
