use crate::{AppState, FileExplorerCommandResult, FileExplorerUiIntent, FileIoManager};
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

    // Whether the context menu was opened on an item or "empty space".
    let target_is_item = path.is_file() || is_directory;

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
    new_or_rename_item_name: Option<String>,
) -> Result<FileExplorerCommandResult, FileExplorerCommandError> {
    use FileExplorerCommand::*;

    let root_path_opt = app_state.get_file_tree().root_path.clone();
    if root_path_opt.is_none() {
        // Can't do anything if there is no root path, so return.
        return Ok(FileExplorerCommandResult { intents: vec![] });
    }

    // Unwrapping here should be fine due to the check above.
    let root_path = root_path_opt.unwrap();

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

    let mut ui_intents: Vec<FileExplorerUiIntent> = Vec::new();

    // NOTE: The `DirectoryRefreshed` UI intent isn't strictly needed in most/all cases since
    // `refresh_dir` reloads the directory items (and the UI side fetches a snapshot of the current
    // state every draw), but is good to have anyway in case we start caching items on the UI side.
    // TODO(scarlet): Implement a more granular refresh for new file/new folder/rename/delete operations?
    // E.g. just add the new item to the tree/update (or remove) the item instead of reloading the whole directory.
    match command {
        NewFile => {
            let name = PathBuf::from(
                new_or_rename_item_name
                    .expect("New item name is required for new file operation")
                    .clone(),
            );

            // Only create the file if it doesn't already exist.
            if !FileIoManager::file_exists(target_directory.join(&name)) {
                FileIoManager::write(target_directory.join(name), "")
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;
            }

            // TODO(scarlet): Select the new file.
            // Refresh the directory so the new file appears in the tree.
            tree.refresh_dir(&target_directory).ok();
            tree.set_expanded(&target_directory);
            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                path: target_directory,
            });
        }
        NewFolder => {
            let name = PathBuf::from(
                new_or_rename_item_name
                    .expect("New item name is required for new folder operation")
                    .clone(),
            );

            // Only create the directory if it doesn't already exist.
            if !FileIoManager::dir_exists(target_directory.join(&name)) {
                FileIoManager::create_directory(target_directory.join(name))
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;
            }

            // TODO(scarlet): Select the new folder.
            // Refresh the directory so the new folder appears in the tree.
            tree.refresh_dir(&target_directory).ok();
            tree.set_expanded(&target_directory);
            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                path: target_directory,
            });
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
            let new_filename = PathBuf::from(
                &new_or_rename_item_name
                    .expect("Rename item name is required for rename operation"),
            );
            let parent_dir = ctx.item_path.parent().ok_or_else(|| {
                FileExplorerCommandError::UnknownCommand(
                    "Cannot rename the file system root".into(),
                )
            })?;
            let new_path = parent_dir.join(&new_filename);

            // Check for naming collisions.
            if FileIoManager::exists(&new_path) {
                return Err(FileExplorerCommandError::IoError(
                    id.to_string(),
                    io::Error::new(
                        io::ErrorKind::AlreadyExists,
                        "A file with that name already exists",
                    ),
                ));
            }

            // Rename the item.
            FileIoManager::rename(&ctx.item_path, &new_path)
                .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;

            // TODO(scarlet): Reselect the renamed item.
            // Refresh the parent directory so the tree reflects the name change.
            tree.refresh_dir(parent_dir).ok();
            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                path: parent_dir.to_path_buf(),
            });
        }
        Delete => {
            // Delete the file or directory, then refresh the parent.
            if ctx.item_path.is_dir() {
                FileIoManager::delete_directory(&ctx.item_path)
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;
            } else {
                FileIoManager::delete_file(&ctx.item_path)
                    .map_err(|error| FileExplorerCommandError::IoError(id.to_string(), error))?;
            }

            if let Some(parent) = ctx.item_path.parent() {
                tree.refresh_dir(parent).ok();
                ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed {
                    path: parent.to_path_buf(),
                });
            }
        }
        Expand => {
            if ctx.item_is_directory && ctx.item_is_expanded {
                tree.set_collapsed(ctx.item_path.clone());
            } else {
                tree.set_expanded(ctx.item_path.clone());
            }
        }
        CollapseAll => {
            tree.collapse_all();
            ui_intents.push(FileExplorerUiIntent::DirectoryRefreshed { path: root_path });
        }
    }

    Ok(FileExplorerCommandResult {
        intents: ui_intents,
    })
}

pub fn get_available_file_explorer_commands(
    ctx: FileExplorerContext,
) -> io::Result<Vec<FileExplorerCommand>> {
    Ok(FileExplorerCommand::all(&ctx))
}
