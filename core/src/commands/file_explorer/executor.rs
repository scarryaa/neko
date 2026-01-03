use crate::{AppState, FileIoManager};
use std::{io, path::PathBuf};

use super::{
    FileExplorerCommand, FileExplorerCommandError, FileExplorerCommandState, FileExplorerContext,
};

pub fn file_explorer_command_state(
    app_state: &AppState,
    path: PathBuf,
) -> Result<FileExplorerCommandState, FileExplorerCommandError> {
    let is_directory = path.is_dir();
    let root_path_opt = app_state.get_file_tree().root_path.clone();
    if root_path_opt.is_none() {
        // Can't do anything if there is no root path, so return.
        return Ok(FileExplorerCommandState::default());
    }

    let root_path = root_path_opt.unwrap();

    // Whether the context menu was opened on an item or "empty space".
    let target_is_item = root_path == path;

    Ok(FileExplorerCommandState {
        can_make_new_file: true,
        can_make_new_folder: true,
        can_reveal_in_system: true,
        can_open_in_terminal: true,
        can_find_in_folder: is_directory,
        can_cut: true,
        can_copy: true,
        can_duplicate: true,
        can_paste: true,
        can_copy_path: true,
        can_copy_relative_path: true,
        can_show_history: !is_directory,
        can_rename: true,
        can_delete: target_is_item,
        can_expand_item: is_directory,
        can_collapse_item: is_directory,
        can_collapse_all: !target_is_item,
    })
}

pub fn run_file_explorer_command(
    app_state: &mut AppState,
    id: &str,
    ctx: &FileExplorerContext,
) -> Result<(), FileExplorerCommandError> {
    use FileExplorerCommand::*;

    let command: FileExplorerCommand = id
        .parse()
        .map_err(|_| FileExplorerCommandError::UnknownCommand(id.to_string()))?;
    let tree = app_state.get_file_tree_mut();

    // Helper to determine where to create new items.
    // If the target is a directory, put it inside. If it's a file, put it in the parent directory.
    let target_directory = if ctx.item_path.is_dir() {
        ctx.item_path.clone()
    } else {
        ctx.item_path
            .parent()
            .map(|p| p.to_path_buf())
            .unwrap_or_else(|| ctx.item_path.clone())
    };

    match command {
        NewFile => {
            let name = PathBuf::from(ctx.new_item_name.clone());

            if FileIoManager::exists(target_directory.join(&name)) {
                FileIoManager::write(target_directory.join(name), "")
                    .map_err(FileExplorerCommandError::IoError)?;
            }

            // Refresh the directory so the new file appears in the tree.
            tree.refresh_dir(&target_directory).ok();
            tree.set_expanded(&target_directory);
        }
        NewFolder => {
            let name = PathBuf::from(ctx.new_item_name.clone());

            if FileIoManager::exists(target_directory.join(&name)) {
                FileIoManager::create_directory(target_directory.join(name))
                    .map_err(FileExplorerCommandError::IoError)?;
            }

            // Refresh the directory so the new folder appears in the tree.
            tree.refresh_dir(&target_directory).ok();
            tree.set_expanded(&target_directory);
        }
        Reveal => {
            // Opens the native OS file explorer pointing to the file.
            let path = &ctx.item_path;

            #[cfg(target_os = "macos")]
            let _ = std::process::Command::new("open")
                .arg("-R")
                .arg(path)
                .spawn();

            #[cfg(target_os = "windows")]
            let _ = std::Process::Command::new("explorer")
                .arg("/select,")
                .arg(path)
                .spawn();

            #[cfg(target_os = "linux")]
            {
                // We try to open the parent dir with xdg-open.
                // TODO(scarlet): Expand on this?
                if let Some(parent) = path.parent() {
                    let _ = std::Process::Command::new("xdg-open").arg(parent).spawn();
                }
            }
        }
        OpenInTerminal => {
            // TODO(scarlet): Implement this once terminal is added.
        }
        FindInFolder => {
            // TODO(scarlet): Implement this once search is added.
        }
        Cut => {
            // Implemented by UI (for now).
        }
        Copy => {
            // Implemented by UI (for now).
        }
        Duplicate => {
            // Implemented by UI (for now).
        }
        Paste => {
            // Implemented by UI (for now).
        }
        CopyPath => {
            // Implemented by UI (for now).
        }
        CopyRelativePath => {
            // Implemented by UI (for now).
        }
        ShowHistory => {
            // TODO(scarlet): Implement this using git history.
        }
        Rename => {
            let new_filename = PathBuf::from(&ctx.rename_item_name);
            let parent_dir = ctx.item_path.parent().ok_or_else(|| {
                FileExplorerCommandError::UnknownCommand(
                    "Cannot rename the file system root".into(),
                )
            })?;
            let new_path = parent_dir.join(&new_filename);

            // Check for naming collisions.
            if FileIoManager::exists(&new_path) {
                return Err(FileExplorerCommandError::IoError(std::io::Error::new(
                    std::io::ErrorKind::AlreadyExists,
                    "A file with that name already exists",
                )));
            }

            // Rename the item.
            FileIoManager::rename(&ctx.item_path, &new_path)
                .map_err(FileExplorerCommandError::IoError)?;

            // Refresh the parent directory so the tree reflects the name change.
            tree.refresh_dir(parent_dir).ok();
            // TODO(scarlet): Reselect the renamed item.
        }
        Delete => {
            // Delete the file or directory, then refresh the parent.
            if ctx.item_path.is_dir() {
                FileIoManager::delete_directory(&ctx.item_path)
                    .map_err(FileExplorerCommandError::IoError)?;
            } else {
                FileIoManager::delete_file(&ctx.item_path)
                    .map_err(FileExplorerCommandError::IoError)?;
            }

            if let Some(parent) = ctx.item_path.parent() {
                tree.refresh_dir(parent).ok();
            }
        }
        Expand => {
            tree.set_expanded(ctx.item_path.clone());
        }
        Collapse => {
            tree.set_collapsed(ctx.item_path.clone());
        }
        CollapseAll => {
            tree.collapse_all();
        }
    }

    Ok(())
}

pub fn get_available_file_explorer_commands() -> io::Result<Vec<FileExplorerCommand>> {
    Ok(FileExplorerCommand::all().to_vec())
}
