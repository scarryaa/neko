use std::{
    io,
    path::{Path, PathBuf},
};

pub type FileResult<T> = io::Result<T>;

/// Thin wrapper around [`std::fs`] IO operations.
///
/// Centralizes file access so editor-level behavior (mocking, validation,
/// path normalization, etc.) can be added or changed without affecting call-sites.
pub struct FileIoManager;

impl FileIoManager {
    pub fn write(path: &Path, content: &str) -> FileResult<()> {
        std::fs::write(path, content)
    }

    pub fn read(path: &Path) -> FileResult<String> {
        std::fs::read_to_string(path)
    }

    pub fn canonicalize(path: &Path) -> FileResult<PathBuf> {
        std::fs::canonicalize(path)
    }
}
