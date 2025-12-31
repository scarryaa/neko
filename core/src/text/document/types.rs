use crate::Buffer;
use std::path::PathBuf;

/// Represents the id associated with a given [`Document`].
#[derive(Eq, Hash, PartialEq, Clone, Copy, Debug)]
pub struct DocumentId(pub u64);

impl DocumentId {
    pub fn take_next(&mut self) -> Self {
        let current_id = *self;
        self.0 = self.0.saturating_add(1);
        current_id
    }
}

/// Represents an open document (file/unsaved buffer) in the editor.
///
/// A `Document` owns its text buffer and associated file metadata (path, title,
/// modified state).
#[derive(Debug)]
pub struct Document {
    pub id: DocumentId,
    pub path: Option<PathBuf>,
    pub title: String,
    pub buffer: Buffer,
    pub modified: bool,
}
