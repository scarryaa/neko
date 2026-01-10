use super::FileIoManager;
use std::{
    io,
    path::{Path, PathBuf},
};

pub type OperationResult<T> = io::Result<T>;

/// Handles various file operations.
///
/// Differs from [`super::FileIoManager`] in that, while the former handles lower-level file
/// operations (like copy, write, read, etc.), `FileOperationsManager` handles slightly more general
/// operations, like pasting directories/files and adjusting target paths.
pub struct FileOperationsManager;

impl FileOperationsManager {
    /// Adjusts the destination path of a paste operation.
    ///
    /// If the provided path is a directory, it is adjusted to target within the
    /// directory.
    ///
    /// If the provided path is a file, it is adjusted to target the parent
    /// directory.
    pub fn get_destination_path<P: AsRef<Path>, Q: AsRef<Path>>(
        source_path: P,
        target_path: Q,
        item_file_name: &str,
    ) -> OperationResult<PathBuf> {
        let source_path = source_path.as_ref();
        let target_path = target_path.as_ref();

        // Determine if a file path or a directory path was provided.
        if target_path.is_dir() {
            let full_source_path =
                source_path.to_string_lossy() + std::path::MAIN_SEPARATOR_STR + item_file_name;
            let target_path_lossy = target_path.to_string_lossy().into_owned();

            // If the source path and the target path are the same, don't adjust the path,
            // so it gets identified as a duplicate operation.
            if full_source_path == target_path_lossy {
                Ok(target_path_lossy.into())
            } else {
                // Otherwise, adjust the path to target within the directory.
                if let Some(source_file_name) =
                    source_path.file_name().map(|path| path.to_string_lossy())
                {
                    Ok(
                        (target_path_lossy + std::path::MAIN_SEPARATOR_STR + &source_file_name)
                            .into(),
                    )
                } else {
                    Err(io::Error::new(
                        io::ErrorKind::NotFound,
                        "Could not get source path file name",
                    ))
                }
            }
        } else {
            // A file path was provided; adjust the path to target the parent directory.
            if let Some(parent_path) = target_path.parent()
                && let Some(source_path_file_name) = source_path.file_name()
            {
                Ok((parent_path.to_string_lossy().into_owned()
                    + std::path::MAIN_SEPARATOR_STR
                    + &source_path_file_name.to_string_lossy())
                    .into())
            } else {
                Err(io::Error::new(
                    io::ErrorKind::NotFound,
                    "Could not get target path parent",
                ))
            }
        }
    }

    // Helper to handle a duplicate operation.
    //
    // Appends " copy" to the target item name until it no longer exists, then proceeds to
    // duplicate the target item.
    pub fn duplicate(item_path: PathBuf) -> OperationResult<PathBuf> {
        const DUPLICATE_SUFFIX: &str = " copy";
        let item_path_string = item_path.as_os_str().to_string_lossy();

        if !item_path.exists() {
            return Err(io::Error::new(
                io::ErrorKind::NotFound,
                "Provided item path does not exist ",
            ));
        }

        let parent_directory_path = item_path.parent();
        let basename = item_path.file_name();
        let suffix = item_path.extension();
        let extension = suffix
            .map(|suffix| ".".to_owned() + &suffix.to_string_lossy())
            .unwrap_or("".to_string());

        let Some(basename) = basename
            .map(|n| n.to_string_lossy())
            .filter(|n| !n.is_empty())
        else {
            return Err(io::Error::new(
                io::ErrorKind::NotFound,
                "Provided item path file name cannot be empty",
            ));
        };

        let mut base_duplicate_name = basename + DUPLICATE_SUFFIX;
        let mut candidate_name = base_duplicate_name.as_ref().to_owned() + &extension;
        let mut destination_path = "".to_string();

        if let Some(parent_directory_path) =
            parent_directory_path.map(|path| path.to_string_lossy())
        {
            destination_path =
                parent_directory_path.to_string() + std::path::MAIN_SEPARATOR_STR + &candidate_name;

            // Append " copy" to the destination file/directory name until it does not overlap with
            // existing items.
            while AsRef::<Path>::as_ref(&destination_path).exists() {
                base_duplicate_name += DUPLICATE_SUFFIX;
                candidate_name = base_duplicate_name.as_ref().to_owned() + &extension;
                destination_path = parent_directory_path.as_ref().to_owned()
                    + std::path::MAIN_SEPARATOR_STR
                    + &candidate_name;
            }

            if item_path.is_dir() {
                // Provided path is a directory.
                if FileIoManager::copy_recursively(item_path.clone(), &destination_path).is_err() {
                    eprintln!("Failed to duplicate directory: {item_path_string}");
                }
            } else {
                // Provided path is a file.
                if FileIoManager::copy_file(item_path.clone(), &destination_path).is_err() {
                    eprintln!("Failed to duplicate file: {item_path_string}");
                }
            }
        }

        Ok(destination_path.into())
    }

    // Helper to handle a paste operation.
    //
    // Handles cases like colliding paths (treated as a duplicate operation), a
    // cut/paste sequence, or a normal copy/paste sequence.
    pub fn handle_paste(
        source_path: PathBuf,
        destination_path: PathBuf,
        is_cut_operation: bool,
        is_for_directory: bool,
    ) -> OperationResult<PathBuf> {
        let mut final_path = Default::default();

        if is_cut_operation {
            // It's a cut operation on an item.
            if FileIoManager::rename(&source_path, &destination_path).is_err() {
                // If the cut operation failed due to collision, duplicate the item and delete the
                // original instead.
                Self::duplicate(source_path).map(|path| final_path = path)?
            } else {
                // If it succeeded, delete the original item.
                match is_for_directory {
                    true => {
                        FileIoManager::remove_directory_all(source_path)?;
                        final_path = destination_path;

                        return Ok(final_path);
                    }
                    false => {
                        FileIoManager::remove_file(source_path)?;
                        final_path = destination_path;

                        return Ok(final_path);
                    }
                }
            }
        } else {
            // If the item already exists, treat it as a duplicate operation.
            if FileIoManager::exists(&destination_path)? {
                Self::duplicate(destination_path).map(|path| final_path = path)?
            } else {
                match is_for_directory {
                    true => {
                        // Copy operation on a directory.
                        FileIoManager::copy_recursively(source_path, destination_path)
                            .map(|_| ())?
                    }
                    false => {
                        // Regular copy operation on a single file.
                        FileIoManager::copy_file(source_path, destination_path).map(|_| ())?
                    }
                }
            }
        }

        Ok(final_path)
    }
}
