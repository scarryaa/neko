use crate::{CursorEntry, Selection};
use std::path::PathBuf;

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
    pub found_id: Option<usize>,
    pub reopened_tab: bool,
    pub scroll_offsets: Option<ScrollOffsets>,
    pub cursors: Option<Vec<CursorEntry>>,
    pub selections: Option<Selection>,
    pub reopen_path: Option<PathBuf>,
}
