use std::{
    fs::{self, ReadDir},
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
    pub fn write<P: AsRef<Path>>(path: P, content: &str) -> FileResult<()> {
        fs::write(path, content)
    }

    pub fn read<P: AsRef<Path>>(path: P) -> FileResult<String> {
        fs::read_to_string(path)
    }

    pub fn read_dir<P: AsRef<Path>>(path: P) -> FileResult<ReadDir> {
        fs::read_dir(path)
    }

    pub fn canonicalize<P: AsRef<Path>>(path: P) -> FileResult<PathBuf> {
        fs::canonicalize(path)
    }

    pub fn create_directory<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::create_dir_all(path)
    }

    pub fn delete_file<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::remove_file(path)
    }

    pub fn delete_directory<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::remove_dir_all(path)
    }

    pub fn rename<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) -> FileResult<()> {
        fs::rename(from, to)
    }

    pub fn exists<P: AsRef<Path>>(path: P) -> bool {
        path.as_ref().exists()
    }
}
