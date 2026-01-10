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
    pub fn write_file<P: AsRef<Path>>(path: P, content: &str) -> FileResult<()> {
        fs::write(path, content)
    }

    pub fn read_file<P: AsRef<Path>>(path: P) -> FileResult<String> {
        fs::read_to_string(path)
    }

    pub fn read_directory<P: AsRef<Path>>(path: P) -> FileResult<ReadDir> {
        fs::read_dir(path)
    }

    pub fn canonicalize<P: AsRef<Path>>(path: P) -> FileResult<PathBuf> {
        fs::canonicalize(path)
    }

    pub fn create_directory_all<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::create_dir_all(path)
    }

    pub fn create_directory<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::create_dir(path)
    }

    pub fn remove_file<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::remove_file(path)
    }

    pub fn remove_directory_all<P: AsRef<Path>>(path: P) -> FileResult<()> {
        fs::remove_dir_all(path)
    }

    pub fn copy_file<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) -> FileResult<()> {
        fs::copy(from, to).map(|_| ())
    }

    pub fn rename<P: AsRef<Path>, Q: AsRef<Path>>(from: P, to: Q) -> FileResult<()> {
        fs::rename(from, to)
    }

    pub fn exists<P: AsRef<Path>>(path: P) -> FileResult<bool> {
        fs::exists(path)
    }

    pub fn directory_exists<P: AsRef<Path>>(path: P) -> bool {
        let path_ref = path.as_ref();
        path_ref.exists() && path_ref.is_dir()
    }

    pub fn file_exists<P: AsRef<Path>>(path: P) -> bool {
        let path_ref = path.as_ref();
        path_ref.exists() && path_ref.is_file()
    }

    pub fn copy_recursively<P: AsRef<Path>, Q: AsRef<Path>>(
        source_path: P,
        destination_path: Q,
    ) -> Result<(), io::Error> {
        if !source_path.as_ref().exists() {
            return Err(io::Error::new(
                io::ErrorKind::NotFound,
                "Source file or directory was not found",
            ));
        }

        if let (Ok(absolute_source_path), Ok(absolute_destination_path)) = (
            std::path::absolute(&source_path),
            std::path::absolute(&destination_path),
        ) {
            // Prevent copying a directory into itself.
            if absolute_source_path == absolute_destination_path {
                return Err(io::Error::new(
                    io::ErrorKind::IsADirectory,
                    "Cannot copy a directory into itself",
                ));
            }

            // Prevent copying a directory into its own subdirectory.
            let source_boundary_string =
                absolute_source_path.to_string_lossy() + std::path::MAIN_SEPARATOR_STR;

            if absolute_destination_path.starts_with(PathBuf::from(source_boundary_string.as_ref()))
            {
                return Err(io::Error::new(
                    io::ErrorKind::IsADirectory,
                    "Cannot copy a directory into its own subdirectory",
                ));
            }

            // Make the destination directory if it does not exist.
            if !destination_path.as_ref().exists()
                && FileIoManager::create_directory_all(&destination_path).is_err()
            {
                return Err(io::Error::other(
                    "Creating the destination directory failed",
                ));
            }

            // Get the list of items in the source directory.
            for entry in FileIoManager::read_directory(&source_path)? {
                let entry = entry?;
                let entry_file_name = entry.file_name();

                let source_path = source_path.as_ref().to_string_lossy()
                    + std::path::MAIN_SEPARATOR_STR
                    + entry_file_name.to_string_lossy();
                let destination_path = destination_path.as_ref().to_string_lossy()
                    + std::path::MAIN_SEPARATOR_STR
                    + entry_file_name.to_string_lossy();

                if AsRef::<Path>::as_ref(source_path.as_ref()).is_dir() {
                    if Self::copy_recursively(source_path.as_ref(), destination_path.as_ref())
                        .is_err()
                    {
                        return Err(io::Error::new(
                            io::ErrorKind::IsADirectory,
                            "Copying directory failed",
                        ));
                    }
                } else if FileIoManager::copy_file(source_path.as_ref(), destination_path.as_ref())
                    .is_err()
                {
                    return Err(io::Error::new(
                        io::ErrorKind::IsADirectory,
                        "Copying file failed",
                    ));
                }
            }
        }

        Ok(())
    }
}
