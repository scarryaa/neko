use crate::{Buffer, DocumentError};
use std::{num::NonZeroU64, path::PathBuf};

/// Represents the id associated with a given [`Document`].
#[derive(Eq, Hash, PartialEq, Clone, Copy, Debug)]
pub struct DocumentId(NonZeroU64);

impl DocumentId {
    pub fn new(id: u64) -> Result<Self, DocumentError> {
        NonZeroU64::new(id)
            .map(Self)
            .ok_or(DocumentError::InvalidId(id))
    }

    pub fn take_next(&mut self) -> Self {
        let current_id = *self;
        self.0 = self.0.saturating_add(1);
        current_id
    }
}

impl From<DocumentId> for u64 {
    fn from(id: DocumentId) -> Self {
        id.0.get()
    }
}

impl From<u64> for DocumentId {
    fn from(id: u64) -> Self {
        DocumentId::new(id).expect("Document id cannot be 0")
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
    pub saved_hash: u32,
    pub saved_revision: usize,
}
