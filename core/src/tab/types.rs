use crate::{CursorEntry, Selection, TabError};
use std::{fmt, num::NonZeroU64, path::PathBuf};

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

impl From<u64> for TabId {
    fn from(id: u64) -> Self {
        TabId::new(id).expect("Tab id cannot be 0")
    }
}

impl fmt::Display for TabId {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{}", self.0)
    }
}

#[derive(Debug, PartialEq, Eq, Clone)]
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
