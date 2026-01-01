use crate::{FileSystemError, FileSystemResult};
use std::{fs::DirEntry, os::unix::ffi::OsStrExt, path::PathBuf, time::UNIX_EPOCH};

/// Represents a node within a [`super::FileTree`], containing various info like the node path,
/// name, size, whether it's a directory, etc.
#[derive(Clone, Debug)]
pub struct FileNode {
    /// The system path of the `FileNode`.
    pub path: PathBuf,
    /// The name of the `FileNode`.
    pub name: String,
    /// Indicates whether the `FileNode` is a directory.
    pub is_dir: bool,
    /// Indicates whether the `FileNode` is considered hidden.
    pub is_hidden: bool,
    /// The size of the `FileNode`.
    pub size: u64,
    /// The modified date of the `FileNode`.
    pub modified: u64,
    /// The "depth" of the `FileNode`, e.g. how nested it is within the folder structure of the [`super::FileTree`].
    pub depth: usize,
}

impl FileNode {
    /// Converts a `DirEntry` into a new `FileNode` result.
    pub fn from_entry(entry: DirEntry, depth: usize) -> FileSystemResult<Self> {
        let path = entry.path();
        let metadata = entry.metadata()?;
        let name = path
            .file_name()
            .ok_or(FileSystemError::MissingName)?
            .to_string_lossy()
            .into_owned();

        let is_dir = metadata.is_dir();

        // A path is considered hidden if any ancestor starts with '.'
        let is_hidden = path.ancestors().any(|a| {
            a.file_name()
                .map(|n| n.as_bytes().first() == Some(&b'.'))
                .unwrap_or(false)
        });

        let size = metadata.len();
        let modified = metadata
            .modified()?
            .duration_since(UNIX_EPOCH)
            .map_err(FileSystemError::BadSystemTime)?
            .as_secs();

        Ok(FileNode {
            path,
            name,
            is_dir,
            is_hidden,
            size,
            modified,
            depth,
        })
    }

    /// Convenience method for cloning the `FileNode` with a different depth.
    pub fn with_depth(&self, depth: usize) -> Self {
        FileNode {
            depth,
            ..self.clone()
        }
    }

    /// Returns the `FileNode` path as a `String`.
    pub fn path_str(&self) -> String {
        self.path.to_string_lossy().into_owned()
    }

    /// Returns the `FileNode` name.
    pub fn name(&self) -> String {
        self.name.clone()
    }
}
